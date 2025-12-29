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

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU
General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "precompiled.h"
#pragma hdrstop

#include "CMDLib.h"

#define PATHSEPERATOR '/'

/*
================
Q_filelength
================
*/
int Q_filelength( FILE* f )
{
	int pos;
	int end;

	pos = ftell( f );
	fseek( f, 0, SEEK_END );
	end = ftell( f );
	fseek( f, pos, SEEK_SET );

	return end;
}

/*
==============
LoadFile
==============
*/
int LoadFile( const char* filename, void** bufferptr )
{
	FILE* f;
	int	  length;
	void* buffer;

	*bufferptr = NULL;

	if( filename == NULL || strlen( filename ) == 0 )
	{
		return -1;
	}

	f = fopen( filename, "rb" );
	if( !f )
	{
		return -1;
	}
	length						= Q_filelength( f );
	buffer						= Mem_ClearedAlloc( length + 1 );
	( ( char* )buffer )[length] = 0;
	if( ( int )fread( buffer, 1, length, f ) != length )
	{
		idLib::Error( "File read failure" );
	}
	fclose( f );

	*bufferptr = buffer;
	return length;
}

/*
==============
DefaultExtension
==============
*/
void DefaultExtension( char* path, char* extension )
{
	char* src;
	//
	// if path doesn't have a .EXT, append extension
	// (extension should include the .)
	//
	src = path + strlen( path ) - 1;

	while( *src != PATHSEPERATOR && src != path )
	{
		if( *src == '.' )
		{
			return; // it has an extension
		}
		src--;
	}

	strcat( path, extension );
}

/*
==============
DefaultPath
==============
*/
void DefaultPath( char* path, char* basepath )
{
	char temp[128];

	if( path[0] == PATHSEPERATOR )
	{
		return; // absolute path location
	}
	strcpy( temp, path );
	strcpy( path, basepath );
	strcat( path, temp );
}

/*
==============
StripFilename
==============
*/
void StripFilename( char* path )
{
	int length;

	length = strlen( path ) - 1;
	while( length > 0 && path[length] != PATHSEPERATOR )
	{
		length--;
	}
	path[length] = 0;
}

/*
==============
StripExtension
==============
*/
void StripExtension( char* path )
{
	int length;

	length = strlen( path ) - 1;
	while( length > 0 && path[length] != '.' )
	{
		length--;
		if( path[length] == '/' )
		{
			return; // no extension
		}
	}
	if( length )
	{
		path[length] = 0;
	}
}
