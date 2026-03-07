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

#include "precompiled.h"
#pragma hdrstop

/*
==============
idDebugSystem::GetStack
==============
*/
uint32 idDebugSystem::GetStack( uint8 *data, int &len ) {
	Sys_CaptureStackTrace( 2, data, len );

	uint32 hash = idStr::Hash( (char *)data, len );
	if ( hash == 0 ) {
		hash = ( uint32 ) - 1;
	}

	return hash;
}

/*
==============
idDebugSystem::DecodeStack
==============
*/
void idDebugSystem::DecodeStack( uint8 *data, int len, idList<debugStackFrame_t, TAG_CRAP> &info ) {
	int cnt = Sys_GetStackTraceFramesCount( data, len );
	info.SetNum( cnt );
	Sys_DecodeStackTrace( data, len, info.Ptr() );
}

/*
==============
idDebugSystem::CleanStack
==============
*/
void idDebugSystem::CleanStack( idList<debugStackFrame_t, TAG_CRAP> &info, bool cutWinMain, bool shortenPath ) {
	if ( shortenPath ) {
		for ( int i = 0; i < info.Num(); i++ ) {
			const char *filename = idDebugSystem::CleanupFileName( info[i].fileName );
			strcpy( info[i].fileName, filename );
		}
	}
	if ( cutWinMain ) {
		int k = 0;
		for ( k = 0; k < info.Num(); k++ ) {
			const debugStackFrame_t &fr = info[k];
			if ( strcmp( fr.functionName, "main" ) == 0 ||
				strcmp( fr.functionName, "WinMain" ) == 0 ||
				strcmp( fr.functionName, "WinMainCRTStartup" ) == 0 ||
				strcmp( fr.functionName, "__tmainCRTStartup" ) == 0 ) {
				break;
			}
		}
		info.Resize( k );
	}
}

/*
==============
idDebugSystem::StringifyStack
==============
*/
void idDebugSystem::StringifyStack( uint32 hash, const debugStackFrame_t *frames, int framesCnt, char *str, int maxLen ) {
	maxLen--;

	if ( hash ) {
		idStr::snPrintf( str, maxLen, "Stack trace (hash = %08X):\n", hash );
		int l = idStr::Length( str );
		maxLen -= l;
		str += l;
	}

	static const int ALIGN_LENGTH = 40;
	for ( int i = 0; i < framesCnt; i++ ) {
		const debugStackFrame_t &fr = frames[i];

		idStr::snPrintf( str, maxLen, "  %s", fr.functionName );
		int l = idStr::Length( str );
		while ( l < maxLen && l < ALIGN_LENGTH ) {
			str[l++] = ' ';
		}
		maxLen -= l;
		str += l;

		idStr::snPrintf( str, maxLen, "  %s:%d\n", fr.fileName, fr.lineNumber );
		l = idStr::Length( str );
		maxLen -= l;
		str += l;
	}

	str[0] = 0;
}

/*
==============
CleanupSourceCodeFileName
==============
*/
const char *CleanupSourceCodeFileName( const char *fileName ) {
    static char newFileNames[4][MAX_STRING_CHARS];
    static int index;

    index = ( index + 1 ) & 3;
    char *path = newFileNames[index];
    strcpy( path, fileName );

    for ( int i = 0; path[i]; i++ ) {
        if ( path[i] == '\\' ) {
            path[i] = '/';
        }
    }

    char *neo = strstr( path, SOURCE_CODE_BASE_FOLDER );
    if ( neo ) {
        path = neo + strlen( SOURCE_CODE_BASE_FOLDER );
    }

    while ( char *topar = strstr( path, "/../" ) ) {
        char *ptr;
        for ( ptr = topar; ptr > path && *( ptr - 1 ) != '/'; ptr-- );
        topar += 4;
        memmove( ptr, topar, strlen( topar ) + 1 );
    }

    if ( path[0] == '/' ) {
        path++;
    }

    return path;
}

