/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2024 Robert Beckebans

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
#include "precompiled.h"
#pragma hdrstop

#define BINARY_CONFIG "binary.conf"

/*
================================================================================================

idZipContainer

================================================================================================
*/

/*
========================
idZipContainer::Init
========================
*/
bool idZipContainer::Init( const char *_fileName ) {
	unzFile			uf;
	int				err;
	unz_global_info64 gi;
	char			filename_inzip[MAX_ZIPPED_FILE_NAME];
	unz_file_info64	file_info;
	int				fs_numHeaderLongs;
	int 			*fs_headerLongs;
	int				len;

	{
		idFileLocal f = fileSystem->OpenExplicitFileRead( _fileName );
		if( !f ) {
			return false;
		}
		f->Seek( 0, FS_SEEK_END );
		len = f->Tell();
	}

	fs_numHeaderLongs = 0;

	uf = unzOpen( _fileName );
	err = unzGetGlobalInfo64( uf, &gi );

	if( err != UNZ_OK ) {
		idLib::Warning( "Unable to open zip file %s", _fileName );
		return false;
	}

	fileName = _fileName;
	zipFileHandle = uf;
	numFileResources = gi.number_entry;
	//TODO store total length = len;

	cacheTable.SetNum( numFileResources );

	unzGoToFirstFile( uf );
	fs_headerLongs = ( int * )Mem_ClearedAlloc( gi.number_entry *sizeof( int ), TAG_IDFILE );
	for( int i = 0; i < numFileResources; i++ ) {
		idZipCacheEntry &rt = cacheTable[ i ];

		err = unzGetCurrentFileInfo64( uf, &file_info, filename_inzip, sizeof( filename_inzip ), NULL, 0, NULL, 0 );
		if( err != UNZ_OK ) {
			break;
		}
		if( file_info.uncompressed_size > 0 ) {
			fs_headerLongs[fs_numHeaderLongs++] = LittleLong( file_info.crc );
		}

		rt.filename = filename_inzip;
		rt.filename.BackSlashesToSlashes();
		rt.filename.ToLower();
		rt.owner = this;

		// store the file position in the zip
		rt.offset = unzGetOffset64( uf );
		rt.length = file_info.uncompressed_size;

		// add the file to the hash
		const int key = cacheHash.GenerateKey( rt.filename, false );
		bool found = false;
		if( !found ) {
			//idLib::Printf( "rez file name: %s\n", rt.filename.c_str() );
			cacheHash.Add( key, i );
		}

		// go to the next file in the zip
		unzGoToNextFile( uf );
	}

	// ignore all binary paks
	int confHash = cacheHash.GenerateKey( BINARY_CONFIG, false );
	for( int index = cacheHash.GetFirst( confHash ); index != idHashIndex::NULL_INDEX; index = cacheHash.GetNext( index ) ) {
		idZipCacheEntry &rtc = cacheTable[ index ];
		if( idStr::Icmp( rtc.filename, BINARY_CONFIG ) == 0 ) {
			zipFileHandle = NULL;
			unzClose( uf );
			cacheTable.Clear();
			Mem_Free( fs_headerLongs );

			return false;
		}
	}

	// check if this is an addon pak
	/*
	pack->addon = false;
	confHash = HashFileName( ADDON_CONFIG );
	for( pakFile = pack->hashTable[confHash]; pakFile; pakFile = pakFile->next )
	{
		if( !FilenameCompare( pakFile->name, ADDON_CONFIG ) )
		{
			pack->addon = true;
			idFile_InZip* file = ReadFileFromZip( pack, pakFile, ADDON_CONFIG );
			// may be just an empty file if you don't bother about the mapDef
			if( file && file->Length() )
			{
				char* buf;
				buf = new char[ file->Length() + 1 ];
				file->Read( ( void* )buf, file->Length() );
				buf[ file->Length() ] = '\0';
				pack->addon_info = ParseAddonDef( buf, file->Length() );
				delete[] buf;
			}
			if( file )
			{
				CloseFile( file );
			}
			break;
		}
	}
	*/

	checksum = MD4_BlockChecksum( fs_headerLongs, 4 * fs_numHeaderLongs );
	checksum = LittleLong( checksum );

	Mem_Free( fs_headerLongs );

	return true;
}

/*
===========
idZipContainer::ReadFileFromZip
===========
*/
idFile_InZip *idZipContainer::OpenFile( const idZipCacheEntry &rt, const char *relativePath ) {
	// set position in pk4 file to the file (in the zip/pk4) we want a handle on
	unzSetOffset64( zipFileHandle, rt.offset );

	// clone handle and assign a new internal filestream to zip file to it
	unzFile uf = unzReOpen( fileName, zipFileHandle );
	if( uf == NULL ) {
		common->FatalError( "Couldn't reopen %s", fileName.c_str() );
	}

	// the following stuff is needed to get the uncompress filesize (for file->fileSize)
	char	filename_inzip[MAX_ZIPPED_FILE_NAME];
	unz_file_info64	file_info;
	int err = unzGetCurrentFileInfo64( uf, &file_info, filename_inzip, sizeof( filename_inzip ), NULL, 0, NULL, 0 );
	if( err != UNZ_OK ) {
		common->FatalError( "Couldn't get file info for %s in %s, pos %llu", relativePath, fileName.c_str(), rt.offset );
	}

	// create idFile_InZip and set fields accordingly
	idFile_InZip *file = new idFile_InZip();
	file->z = uf;
	file->name = relativePath;
	file->fullPath = fileName + "/" + relativePath;
	file->zipFilePos = rt.offset;
	file->fileSize = file_info.uncompressed_size;

	return file;
}

