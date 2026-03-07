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

#ifndef __FILE_ZIP_H__
#define __FILE_ZIP_H__

/*
==============================================================

  Zip containers based off idResourceContainer and dhewm3

==============================================================
*/
#define MAX_ZIPPED_FILE_NAME	2048

class idZipContainer;

class idZipCacheEntry {
  public:
	idZipCacheEntry() {
		Clear();
	}
	void Clear() {
		filename.Empty();
		offset = 0;
		length = 0;
		owner = NULL;
	}

	// part of .pk4 file format
	idStrStatic< MAX_ZIPPED_FILE_NAME >	filename;
#ifdef _WIN32
	unsigned __int64	offset;
	unsigned __int64	length;		// uncompressed size
#else
	unsigned long long int	offset;
	unsigned long long int	length;		// uncompressed size
#endif

	// helpers only in memory
	idZipContainer *owner;
};

class idZipContainer {
	friend class	idFileSystemLocal;
  public:
	idZipContainer() {
		zipFileHandle = NULL;
		checksum = 0;
		numFileResources = 0;
	}
	~idZipContainer() {
		unzClose( zipFileHandle );
		cacheTable.Clear();
	}
	bool Init( const char *fileName );
	idFile_InZip *OpenFile( const idZipCacheEntry &rt, const char *relativePath );

	const char *GetFileName() const {
		return fileName.c_str();
	}

	int GetNumFileResources() const {
		return numFileResources;
	}

	int	GetChecksum() const {
		return checksum;
	}
  private:
	idStrStatic< 256 >	fileName;			// containst the full OS path unlike idResourcesContainer
	unzFile 			zipFileHandle;		// open file handle
	int					checksum;
	int					numFileResources;		// number of file resources in this container
	idList< idZipCacheEntry, TAG_RESOURCE>	cacheTable;
	idHashIndex			cacheHash;
};


#endif /* !__FILE_ZIP_H__ */
