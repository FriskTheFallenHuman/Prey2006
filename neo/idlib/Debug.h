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

#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <stdint.h>

const char *CleanupSourceCodeFileName( const char *fileName );

// some utilities for getting debug information in runtime
class idDebugSystem {
public:
	// captures call stack of function calling this one into specified binary blob
	// len is: size of data buffer refore call, size of written data after call
	// returns nonzero hash code of the call stack (same call stacks have equal hash codes)
	static uint32 GetStack( uint8 *data, int &len );

	// decodes previously captured call stack as an array of stack frames
	// most recent stack frame goes first in the array
	static void DecodeStack( uint8 *data, int len, idList<debugStackFrame_t, TAG_CRAP> &info );

	// cleans the stack trace according to these rules:
	//   cutWinMain: remove all stack frames above CRT "main" inclusive
	//   shortenPath: remove path prefix ending at "doom3bfg" to make path relative to source code root
	static void CleanStack( idList<debugStackFrame_t, TAG_CRAP>& info, bool cutWinMain = true, bool shortenPath = true);

	// converts given call stack (array of stack frames) into readable string
	// str and maxLen specify the receiving string buffer
	static void StringifyStack( uint32 hash, const debugStackFrame_t *frames, int framesCnt, char *str, int maxLen );

	// caller must copy the returned string immediately!
	// note: does not allocate any memory in anyway
	static const char *CleanupFileName( const char *fileName ) { return CleanupSourceCodeFileName( fileName ); }
};

#endif /* !__DEBUG_H__ */
