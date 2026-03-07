/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

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
===============
idEFXFile::idEFXFile
===============
*/
idEFXFile::idEFXFile() {
}

/*
===============
idEFXFile::Clear
===============
*/
void idEFXFile::Clear() {
	effects.DeleteContents( true );
}

/*
===============
idEFXFile::~idEFXFile
===============
*/
idEFXFile::~idEFXFile() {
	Clear();
}

/*
===============
idEFXFile::FindEffect
===============
*/
bool idEFXFile::FindEffect( idStr & name, idSoundEffect ** effect, int * index ) {
	for ( int i = 0; i < effects.Num(); i++ ) {
		if ( ( effects[i] ) && ( effects[i]->name == name ) ) {
			*effect = effects[i];
			*index = i;
			return true;
		}
	}

	return false;
}

/*
===============
idEFXFile::ReadEffect
===============
*/
bool idEFXFile::ReadEffect( idLexer & src, idSoundEffect * effect ) {
	idToken name, token;

	if ( !src.ReadToken( &token ) ) {
		return false;
	}

	// reverb effect
	if ( token == "reverb" ) {
		bool ret = soundSystemLocal.hardware->ParseEAXEffects( src, name, token, effect );
		return ret;
	} else {
		// other effect (not supported at the moment)
		src.Error( "idEFXFile::ReadEffect: Unknown effect definition" );
	}

	return false;
}


/*
===============
idEFXFile::LoadFile
===============
*/
bool idEFXFile::LoadFile( const char * filename, bool OSPath ) {
	idLexer src( LEXFL_NOSTRINGCONCAT );
	idToken token;

	src.LoadFile( filename, OSPath );
	if ( !src.IsLoaded() ) {
		return false;
	}

	if ( !src.ExpectTokenString( "Version" ) ) {
		return false;
	}

	if ( src.ParseInt() != 1 ) {
		src.Error( "idEFXFile::LoadFile: Unknown file version" );
		return false;
	}

	while ( !src.EndOfFile() ) {
		idSoundEffect *effect = new idSoundEffect;
		if ( ReadEffect( src, effect ) ) {
			effects.Append( effect );
		}
	};

	return true;
}

/*
===============
idEFXFile::UnloadFile
===============
*/
void idEFXFile::UnloadFile() {
	Clear();
}
