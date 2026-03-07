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

#include "tr_local.h"

#define FILESIZE_fontInfo_t (20548)

static int fdOffset;
static byte	*fdFile;

/*
============
readInt
============
*/
int readInt() {
	int i = fdFile[fdOffset]+(fdFile[fdOffset+1]<<8)+(fdFile[fdOffset+2]<<16)+(fdFile[fdOffset+3]<<24);
	fdOffset += 4;
	return i;
}

/*
============
readFloat
============
*/
float readFloat() {
	float f;
	memcpy(&f, &fdFile[fdOffset], sizeof(float));
	fdOffset += sizeof(float);
	return LittleFloat(f);
}

/*
============
RegisterFont

Loads 3 point sizes, 12, 24, and 48
============
*/
bool idRenderSystemLocal::RegisterFont( const char *fontName, fontInfoEx_t &font ) {
	void *faceData;
	ID_TIME_T ftime;
	int i, len, fontCount;
	char name[1024];

	int pointSize = 12;

	assert(sizeof(float) == 4 && "if this is false, readFloat() won't work");

	memset( &font, 0, sizeof( font ) );

	for ( fontCount = 0; fontCount < 3; fontCount++ ) {

		if ( fontCount == 0) {
			pointSize = 12;
		} else if ( fontCount == 1 ) {
			pointSize = 24;
		} else {
			pointSize = 48;
		}
		// we also need to adjust the scale based on point size relative to 48 points as the ui scaling is based on a 48 point font
		float glyphScale = 1.0f;		// change the scale to be relative to 1 based on 72 dpi ( so dpi of 144 means a scale of .5 )
		glyphScale *= 48.0f / pointSize;

		idStr::snPrintf( name, sizeof(name), "%s/fontImage_%i.dat", fontName, pointSize );

		fontInfo_t *outFont;
		if ( fontCount == 0 ) {
			outFont = &font.fontInfoSmall;
		}
		else if ( fontCount == 1 ) {
			outFont = &font.fontInfoMedium;
		}
		else {
			outFont = &font.fontInfoLarge;
		}

		idStr::Copynz( outFont->name, name, sizeof( outFont->name ) );

		len = fileSystem->ReadFile( name, NULL, &ftime );
		if ( len != FILESIZE_fontInfo_t ) {
			common->Warning( "RegisterFont: couldn't find font: '%s'", name );
			return false;
		}

		fileSystem->ReadFile( name, &faceData, &ftime );
		fdOffset = 0;
		fdFile = reinterpret_cast<unsigned char*>(faceData);
		for( i = 0; i < GLYPHS_PER_FONT; i++ ) {
			outFont->glyphs[i].height		= readInt();
			outFont->glyphs[i].top			= readInt();
			outFont->glyphs[i].bottom		= readInt();
			outFont->glyphs[i].pitch		= readInt();
			outFont->glyphs[i].xSkip		= readInt();
			outFont->glyphs[i].imageWidth	= readInt();
			outFont->glyphs[i].imageHeight	= readInt();
			outFont->glyphs[i].s			= readFloat();
			outFont->glyphs[i].t			= readFloat();
			outFont->glyphs[i].s2			= readFloat();
			outFont->glyphs[i].t2			= readFloat();
			/* font.glyphs[i].glyph			= */ readInt();
			//FIXME: the +6, -6 skips the embedded fonts/
			memcpy( outFont->glyphs[i].shaderName, &fdFile[fdOffset + 6], 32 - 6 );
			fdOffset += 32;
		}
		outFont->glyphScale = readFloat();

		int mw = 0;
		int mh = 0;
		for ( i = GLYPH_START; i < GLYPH_END; i++ ) {
			idStr::snPrintf(name, sizeof(name), "%s/%s", fontName, outFont->glyphs[i].shaderName);
			outFont->glyphs[i].glyph = declManager->FindMaterial(name);
			outFont->glyphs[i].glyph->SetSort( SS_GUI );
			if ( mh < outFont->glyphs[i].height ) {
				mh = outFont->glyphs[i].height;
			}
			if ( mw < outFont->glyphs[i].xSkip ) {
				mw = outFont->glyphs[i].xSkip;
			}
		}
		if ( fontCount == 0 ) {
			font.maxWidthSmall = mw;
			font.maxHeightSmall = mh;
		} else if ( fontCount == 1 ) {
			font.maxWidthMedium = mw;
			font.maxHeightMedium = mh;
		} else {
			font.maxWidthLarge = mw;
			font.maxHeightLarge = mh;
		}
		fileSystem->FreeFile( faceData );
	}

	return true ;
}