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
===================
idMapReverb::idMapReverb
===================
*/
idMapReverb::idMapReverb( void ) {
}

/*
===================
idMapReverb::idMapReverb
===================
*/
idMapReverb::~idMapReverb( void ) {
}

/*
===================
idMapReverb::Init
===================
*/
void idMapReverb::Init( void ) {
	Clear();
}

/*
===================
idMapReverb::ParseItem
===================
*/
bool idMapReverb::ParseItem( idLexer &src, idReverbItem_t &item ) const
{
	if ( !src.ExpectTokenString( "{" ) ) {
		return false;
	}

	item.areaNum = src.ParseInt();
	idToken name;
	src.ReadToken( &name );
	item.efxName = name;

	if ( !src.ExpectTokenString( "}" ) ) {
		return false;
	}

	return true;
}

/*
===================
idMapReverb::LoadFile
===================
*/
bool idMapReverb::LoadFile( const char *filename, bool OSPath ) {
	Init();

	idLexer	src;
	src.LoadFile( filename, OSPath );

	if ( !src.IsLoaded() ) {
		return false;
	}

	fileName = filename;
	src.SetFlags( DECL_LEXER_FLAGS );

	return ParseReverb( src );
}

/*
===================
idMapReverb::ParseReverb
===================
*/
bool idMapReverb::ParseReverb( idLexer &src ) {
	idToken token;
	src.SkipUntilString( "{" );

	while ( !src.EndOfFile() ) {
		if ( !src.ReadToken( &token ) ) {
			break;
		}

		if ( !token.Icmp( "}" ) ) {
			break;
		}

		if ( token.Icmp( "{" ) ) {
			items.Clear();
			return false;
		}

		src.UnreadToken( &token );
		idReverbItem_t item;
		if ( !ParseItem( src, item ) ) {
			items.Clear();
			return false;
		}
		EFXprintf( "map EFX: read area %d -> %s\n", item.areaNum, item.efxName.c_str() );
		items.Append( item );
	}
	EFXprintf( "map EFX: load areas %d\n", items.Num() );

	return true;
}

/*
===================
idMapReverb::Append
===================
*/
bool idMapReverb::Append( int area, const char *name, bool over ) {
	int index;

	index = GetAreaIndex( area );
	if ( index >= 0 ) {
		idReverbItem_t &item = items[index];
		if ( !over ) {
			common->Warning( "Area %d has exists with efx name %s", area, item.efxName.c_str() );
			return false;
		} else {
			item.efxName = name;
			return true;
		}
	} else {
		idReverbItem_t item;
		item.areaNum = area;
		item.efxName = name;
		items.Append( item );
		return true;
	}
}

/*
===================
idMapReverb::LoadMap
===================
*/
int idMapReverb::LoadMap( const char *mapName, const char *filterName ) {
	idStr filename = GetMapFileName( mapName, filterName );

	EFXprintf( "map EFX: load reverb file %s -> %s\n", mapName, filename.c_str() );

	if( LoadFile( filename ) ) {
		return items.Num();
	} else {
		return -1;
	}
}

/*
===================
idMapReverb::GetMapFileName
===================
*/
idStr idMapReverb::GetMapFileName( const char *mapName, const char *filterName ) {
	(void)filterName;
	idStr name;

	name += "maps";
	name.AppendPath( mapName );
	name.SetFileExtension( ".reverb" );

	return name;
}

/*
===================
idMapReverb::GetAreaIndex
===================
*/
int idMapReverb::GetAreaIndex( int area ) const {
	for ( int i = 0; i < items.Num(); i++ ) {
		if ( items[i].areaNum == area ) {
			return i;
		}
	}

	return -1;
}

/*
===================
idMapReverb::Clear
===================
*/
void idMapReverb::Clear( void ) {
	fileName = "";
	items.Clear();
}
