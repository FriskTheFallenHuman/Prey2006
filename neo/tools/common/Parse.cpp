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

#include "Parse.h"

char		token[MAXTOKEN];
bool		unget;
const char* script_p;
int			scriptline;

/*
==============
StartTokenParsing

Initializes the token parsing process by setting the script pointer and line number.
==============
*/
void		StartTokenParsing( const char* data )
{
	scriptline = 1;
	script_p   = data;
	unget	   = false;
}

/*
==============
GetToken

Retrieves the next token from the script. Handles spaces, new lines, and comments.
Returns true if a token is successfully retrieved.
==============
*/
bool GetToken( bool crossline )
{
	if( unget )
	{
		unget = false;
		return true;
	}

	// Skip spaces and handle new lines
	while( *script_p <= 32 )
	{
		if( !*script_p )
		{
			if( !crossline )
			{
				common->Warning( "Line %i is incomplete [01]\n", scriptline );
			}
			return false;
		}
		if( *script_p++ == '\n' )
		{
			if( !crossline )
			{
				common->Warning( "Line %i is incomplete [02]\n", scriptline );
			}
			scriptline++;
		}
	}

	// Handle comments
	if( script_p[0] == '/' && script_p[1] == '/' )
	{
		if( !crossline )
		{
			common->Warning( "Line %i is incomplete [03]\n", scriptline );
		}
		while( *script_p++ != '\n' )
		{
			if( !*script_p )
			{
				if( !crossline )
				{
					common->Warning( "Line %i is incomplete [04]\n", scriptline );
				}
				return false;
			}
		}
		return GetToken( crossline );
	}

	// Copy token
	char* token_p = token;
	if( *script_p == '"' )
	{
		script_p++;
		while( *script_p != '"' )
		{
			if( !*script_p )
			{
				printf( "EOF inside quoted token\n" );
				return false;
			}
			*token_p++ = *script_p++;
			if( token_p == &token[MAXTOKEN] )
			{
				printf( "Token too large on line %i\n", scriptline );
				return false;
			}
		}
		script_p++;
	}
	else
	{
		while( *script_p > 32 )
		{
			*token_p++ = *script_p++;
			if( token_p == &token[MAXTOKEN] )
			{
				printf( "Token too large on line %i\n", scriptline );
				return false;
			}
		}
	}

	*token_p = '\0';
	return true;
}

/*
==============
UngetToken

Sets the unget flag to true, indicating that the last token should be re-read.
==============
*/
void UngetToken()
{
	unget = true;
}

/*
==============
TokenAvailable

Returns true if there is another token on the line.
==============
*/
bool TokenAvailable()
{
	const char* search_p = script_p;
	while( *search_p <= 32 )
	{
		if( *search_p == '\n' || *search_p == 0 )
		{
			return false;
		}
		search_p++;
	}
	return *search_p != ';';
}
