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

#include "roq.h"

void R_LoadImage( const char *name, byte **pic, int *width, int *height, ID_TIME_T *timestamp, bool makePowerOf2 );

NSBitmapImageRep::NSBitmapImageRep() {
	bmap = NULL;
	width = 0;
	height = 0;
	timestamp = 0;
}

NSBitmapImageRep::NSBitmapImageRep( const char *filename ) {

	R_LoadImage( filename, &bmap, &width, &height, &timestamp, false );
	if (!width || !height) {
		common->FatalError( "roqvq: unable to load image %s\n", filename );
	}
}

NSBitmapImageRep::NSBitmapImageRep( int wide, int high ) {
	bmap = (byte *)Mem_ClearedAlloc( wide * high * 4, TAG_ROQ );
	width = wide;
	height = high;
}

void R_StaticFree( void *data );

NSBitmapImageRep::~NSBitmapImageRep() {
	R_StaticFree( bmap );
	bmap = NULL;
}

int NSBitmapImageRep::samplesPerPixel() {
	return 4;
}

int NSBitmapImageRep::pixelsWide() {
	return width;
}

int NSBitmapImageRep::pixelsHigh() {
	return height;
}

byte * NSBitmapImageRep::bitmapData() {
	return bmap;
}

bool NSBitmapImageRep::hasAlpha() {
	return false;
}

bool NSBitmapImageRep::isPlanar() {
	return false;
}

NSBitmapImageRep &NSBitmapImageRep::operator=( const NSBitmapImageRep &a ) {

	// check for assignment to self
	if ( this == &a ) {
		return *this;
	}

	if (bmap) {
		Mem_Free(bmap);
	}
	bmap	= (byte *)Mem_Alloc( a.width * a.height * 4, TAG_ROQ );
	memcpy( bmap, a.bmap, a.width * a.height * 4 );
	width = a.width;
	height = a.height;
	timestamp = a.timestamp;

	return *this;
}
