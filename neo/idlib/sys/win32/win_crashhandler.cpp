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

#pragma warning(push)
#pragma warning(disable: 4091)
#include "dbghelp.h"
#pragma warning(pop)

/*
====================
Sys_CaptureStackTrace
====================
*/
void Sys_CaptureStackTrace( int ignoreFrames, uint8 *data, int &len ) {
	int cnt = CaptureStackBackTrace( ignoreFrames, len / sizeof( PVOID ), (PVOID *)data, NULL );
	len = cnt * sizeof( PVOID );
}

/*
====================
Sys_GetStackTraceFramesCount
====================
*/
int Sys_GetStackTraceFramesCount( uint8 *data, int len ) {
	return len / sizeof( PVOID );
}

/*
====================
Sys_GetStackTraceFramesCount
====================
*/
static bool AreSymbolsInitialized = false;
void Sys_DecodeStackTrace( uint8 *data, int len, debugStackFrame_t *frames ) {
	//interpret input blob as array of addresses
	PVOID *addresses = (PVOID *)data;
	int framesCount = Sys_GetStackTraceFramesCount( data, len );

	//fill output with zeros
	memset( frames, 0, framesCount * sizeof( frames[0] ) );

	HANDLE hProcess = GetCurrentProcess();
	if ( !AreSymbolsInitialized ) {
		AreSymbolsInitialized = true;
		SymInitialize( hProcess, NULL, TRUE );
	}

	//allocate symbol structures
	int buff[( sizeof( IMAGEHLP_SYMBOL64 ) + sizeof( frames[0].functionName ) ) / 4 + 1] = { 0 };
	IMAGEHLP_SYMBOL64 *symbol = (IMAGEHLP_SYMBOL64 *)buff;
	symbol->SizeOfStruct = sizeof( IMAGEHLP_SYMBOL64 );
	symbol->MaxNameLength = sizeof( frames[0].functionName ) - 1;
	IMAGEHLP_LINE64 line = {0};
	line.SizeOfStruct = sizeof( IMAGEHLP_LINE64 );

	for ( int i = 0; i < framesCount; i++ ) {
		frames[i].pointer = addresses[i];
		sprintf( frames[i].functionName, "[%p]", frames[i].pointer );	//in case PDB not found

		if ( !addresses[i] ) {
			continue;	// null function?
		}

		if ( !SymGetSymFromAddr64( hProcess, DWORD64( addresses[i] ), NULL, symbol ) ) {
			continue;	// cannot get symbol
		}

		idStr::Copynz( frames[i].functionName, symbol->Name, sizeof( frames[0].functionName ) );

		DWORD displacement = DWORD( -1 );
		if ( !SymGetLineFromAddr64( hProcess, DWORD64( addresses[i] ), &displacement, &line ) ) {
			continue;	// no code line info
		}

		idStr::Copynz( frames[i].fileName, line.FileName, sizeof( frames[0].fileName ) );
		frames[i].lineNumber = line.LineNumber;
	}
}