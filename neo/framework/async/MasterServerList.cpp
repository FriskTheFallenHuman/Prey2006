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

#include "MasterServerList.h"

idCVar net_debugMasterservers( "net_debugMasterservers", "0", CVAR_BOOL | CVAR_SOUND | CVAR_NOCHEAT, "Enable/Disable Console Debug for master server list system" );

bool idMasterServerDecl::LoadFile( const char *fileName, bool OSPath ) {
	idLexer src;

	src.SetFlags(DECL_LEXER_FLAGS);
	src.LoadFile(fileName, OSPath);

	if ( !src.IsLoaded() ) {
		return false;
	}

	while ( !src.EndOfFile() ) {
		idMasterServer *list = new idMasterServer;
		if ( ReadList( src, list ) ) {
			masterservers.Append( list );
		}
	};

	return true;
}

bool idMasterServerDecl::FindServer( const char *name, idMasterServer **server ) {
	for ( int i = 0; i < masterservers.Num(); i++ ) {
		if ( !idStr::Icmp( name, masterservers[i]->GetName().c_str() ) ) {
			*server = masterservers[i];
			return true;
		}
	}
	return false;
}

idMasterServer *idMasterServerDecl::GetServerByIndex( int index ) {
	if ( index < 0 || index >= masterservers.Num() ) {
		return nullptr;
	}
	return masterservers[ index ];
}

int idMasterServerDecl::GetServerCount( void ) {
	return masterservers.Num();
}

void idMasterServerDecl::Clear() {
	masterservers.DeleteContents( true );
}

bool idMasterServerDecl::ReadList( idLexer &src, idMasterServer *entry ) {
	idToken name, token;

	src.SkipUntilString("{");

	while ( 1 ) {
		if ( !src.ReadToken( &token ) ) {
			return false;
		}

		if ( !token.Icmp( "}" ) ) {
			break;
		}

		if ( !token.Icmp( "name" ) ) {
			src.ParseRestOfLine( token );
			entry->SetName( token.c_str() );
		} else if ( !token.Icmp( "address" ) ) {
			src.ParseRestOfLine( token );
			entry->SetAddress( token.c_str() );
		} else if ( !token.Icmp( "port" ) ) {
			src.ParseRestOfLine( token );
			entry->SetPorts( token.c_str() );
		}

	}

	return true;
}
