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

#include "tr_local.h"

#ifdef ID_BUILD_FREETYPE
	#include <ft2build.h>
	#include FT_FREETYPE_H
	//#include FT_ERRORS_H
	#include FT_SYSTEM_H
	#include FT_IMAGE_H
	#include FT_OUTLINE_H

	#define _FLOOR(x)  ((x) & -64)
	#define _CEIL(x)   (((x)+63) & -64)
	#define _TRUNC(x)  ((x) >> 6)

	FT_Library ftLibrary = NULL;
#endif // ID_BUILD_FREETYPE

const int FONT_SIZE = 256;

#ifdef ID_BUILD_FREETYPE
/*
============
R_GetGlyphInfo
============
*/
void R_GetGlyphInfo(FT_GlyphSlot glyph, int *left, int *right, int *width, int *top, int *bottom, int *height, int *pitch) {

	*left  = _FLOOR( glyph->metrics.horiBearingX );
	*right = _CEIL( glyph->metrics.horiBearingX + glyph->metrics.width );
	*width = _TRUNC(*right - *left);

	*top    = _CEIL( glyph->metrics.horiBearingY );
	*bottom = _FLOOR( glyph->metrics.horiBearingY - glyph->metrics.height );
	*height = _TRUNC( *top - *bottom );
	*pitch  = ( *width+3) & -4;
}

/*
============
R_RenderGlyph
============
*/
FT_Bitmap *R_RenderGlyph(FT_GlyphSlot glyph, glyphInfo_t* glyphOut) {
	FT_Bitmap  *bit2;
	int left, right, width, top, bottom, height, pitch, size;

	R_GetGlyphInfo(glyph, &left, &right, &width, &top, &bottom, &height, &pitch);

	if ( glyph->format == FT_Glyph_Format::FT_GLYPH_FORMAT_OUTLINE ) {
		size   = pitch*height;

		bit2 = ( FT_Bitmap* )Mem_Alloc(sizeof(FT_Bitmap));

		bit2->width      = width;
		bit2->rows       = height;
		bit2->pitch      = pitch;
		bit2->pixel_mode = FT_Pixel_Mode::FT_PIXEL_MODE_GRAY;
		//bit2->pixel_mode = ft_pixel_mode_mono;
		bit2->buffer     = ( byte* )Mem_Alloc(pitch*height);
		bit2->num_grays = 256;

		memset( bit2->buffer, 0, size );

		FT_Outline_Translate( &glyph->outline, -left, -bottom );

		FT_Outline_Get_Bitmap( ftLibrary, &glyph->outline, bit2 );

		glyphOut->height = height;
		glyphOut->pitch = pitch;
		glyphOut->top = (glyph->metrics.horiBearingY >> 6) + 1;
		glyphOut->bottom = bottom;

		return bit2;
	}
	else {
		common->Printf( "Non-outline fonts are not supported\n" );
	}
	return NULL;
}

/*
============
RE_ConstructGlyphInfo
============
*/
static glyphInfo_t *RE_ConstructGlyphInfo( unsigned char *imageOut, int *xOut, int *yOut, int *maxHeight, FT_Face face, const unsigned char c, bool calcHeight ) {
	int i;
	static glyphInfo_t glyph;
	unsigned char *src, *dst;
	float scaled_width, scaled_height;
	FT_Bitmap *bitmap = NULL;

	memset(&glyph, 0, sizeof(glyphInfo_t));
	// make sure everything is here
	if (face != NULL) {
		FT_Load_Glyph(face, FT_Get_Char_Index( face, c), FT_LOAD_DEFAULT );
		bitmap = R_RenderGlyph(face->glyph, &glyph);
		if (bitmap) {
			glyph.xSkip = (face->glyph->metrics.horiAdvance >> 6) + 1;
		} else {
			return &glyph;
		}

		if (glyph.height > *maxHeight) {
			*maxHeight = glyph.height;
		}

		if (calcHeight) {
			Mem_Free(bitmap->buffer);
			Mem_Free(bitmap);
			return &glyph;
		}

		/*
				// need to convert to power of 2 sizes so we do not get
				// any scaling from the gl upload
				for (scaled_width = 1 ; scaled_width < glyph.pitch ; scaled_width<<=1)
					;
				for (scaled_height = 1 ; scaled_height < glyph.height ; scaled_height<<=1)
					;
		*/

		scaled_width = glyph.pitch;
		scaled_height = glyph.height;

		// we need to make sure we fit
		if (*xOut + scaled_width + 1 >= (FONT_SIZE-1)) {
			if (*yOut + (*maxHeight + 1 ) * 2 >= (FONT_SIZE-1)) {
				*yOut = -1;
				*xOut = -1;
				Mem_Free(bitmap->buffer);
				Mem_Free(bitmap);
				return &glyph;
			} else {
				*xOut = 0;
				*yOut += *maxHeight + 1;
			}
		} else if (*yOut + *maxHeight + 1 >= (FONT_SIZE-1)) {
			*yOut = -1;
			*xOut = -1;
			Mem_Free(bitmap->buffer);
			Mem_Free(bitmap);
			return &glyph;
		}

		src = bitmap->buffer;
		dst = imageOut + (*yOut * FONT_SIZE) + *xOut;

		if (bitmap->pixel_mode == FT_Pixel_Mode::FT_PIXEL_MODE_MONO) {
			for (i = 0; i < glyph.height; i++) {
				int j;
				unsigned char *_src = src;
				unsigned char *_dst = dst;
				unsigned char mask = 0x80;
				unsigned char val = *_src;
				for (j = 0; j < glyph.pitch; j++) {
					if (mask == 0x80) {
						val = *_src++;
					}
					if (val & mask) {
						*_dst = 0xff;
					}
					mask >>= 1;

					if ( mask == 0 ) {
						mask = 0x80;
					}
					_dst++;
				}

				src += glyph.pitch;
				dst += FONT_SIZE;

			}
		} else {
			for (i = 0; i < glyph.height; i++) {
				memcpy( dst, src, glyph.pitch );
				src += glyph.pitch;
				dst += FONT_SIZE;
			}
		}

		// we now have an 8 bit per pixel grey scale bitmap
		// that is width wide and pf->ftSize->metrics.y_ppem tall

		glyph.imageHeight = scaled_height;
		glyph.imageWidth = scaled_width;
		glyph.s = (float)*xOut / FONT_SIZE;
		glyph.t = (float)*yOut / FONT_SIZE;
		glyph.s2 = glyph.s + (float)scaled_width / FONT_SIZE;
		glyph.t2 = glyph.t + (float)scaled_height / FONT_SIZE;

		*xOut += scaled_width + 1;
	}

	Mem_Free(bitmap->buffer);
	Mem_Free(bitmap);

	return &glyph;
}
#endif // ID_BUILD_FREETYPE

/*
============
FinalizeFontInfoEx
============
*/
static void FinalizeFontInfoEx( const char *fontName, fontInfoEx_t &font, fontInfo_t *outFont, int fontCount ) {
	idStr name;
	int mw = 0;
	int mh = 0;

	for (int i = GLYPH_START; i < GLYPH_END; i++) {
		if (idStr::Cmpn( outFont->glyphs[i].shaderName, "fonts/", 6 ) == 0) {
			sprintf( name, "%s/%s", fontName, outFont->glyphs[i].shaderName + 6 );
		} else {
			sprintf( name, "%s/%s", fontName, outFont->glyphs[i].shaderName );
		}

		outFont->glyphs[i].glyph = declManager->FindMaterial( name );
		outFont->glyphs[i].glyph->SetSort( SS_GUI );

		if (mh < outFont->glyphs[i].height) {
			mh = outFont->glyphs[i].height;
		}

		if (mw < outFont->glyphs[i].xSkip) {
			mw = outFont->glyphs[i].xSkip;
		}
	}

	if (fontCount == 0) {
		font.maxWidthSmall = mw;
		font.maxHeightSmall = mh;
	} else if (fontCount == 1) {
		font.maxWidthMedium = mw;
		font.maxHeightMedium = mh;
	} else {
		font.maxWidthLarge = mw;
		font.maxHeightLarge = mh;
	}
}

/*
============
LoadFontInfo
============
*/
static bool LoadFontInfo( const char *name, fontInfo_t *outFont ) {
	idFile *file = fileSystem->OpenFileRead( name );

	if (file == NULL) {
		return false;
	}

	for (int i = 0; i < GLYPHS_PER_FONT; i++) {
		file->ReadInt( outFont->glyphs[i].height );
		file->ReadInt( outFont->glyphs[i].top );
		file->ReadInt( outFont->glyphs[i].bottom );
		file->ReadInt( outFont->glyphs[i].pitch );
		file->ReadInt( outFont->glyphs[i].xSkip );
		file->ReadInt( outFont->glyphs[i].imageWidth );
		file->ReadInt( outFont->glyphs[i].imageHeight );
		file->ReadFloat( outFont->glyphs[i].s );
		file->ReadFloat( outFont->glyphs[i].t );
		file->ReadFloat( outFont->glyphs[i].s2 );
		file->ReadFloat( outFont->glyphs[i].t2 );
		int junk; /* font.glyphs[i].glyph */
		file->ReadInt( junk );

		for (int j = 0; j < 32; j++) {
			file->ReadChar( outFont->glyphs[i].shaderName[j] );
		}
	}
	file->ReadFloat( outFont->glyphScale );

	fileSystem->CloseFile( file );

	return true;
}

/*
============
SaveFontInfo
============
*/
static bool SaveFontInfo( const char *name, const fontInfo_t *outFont ) {
	idFile *file = fileSystem->OpenFileWrite( name, "fs_basepath" );

	if (file == NULL) {
		return false;
	}

	for (int i = 0; i < GLYPHS_PER_FONT; i++) {
		file->WriteInt( outFont->glyphs[i].height );
		file->WriteInt( outFont->glyphs[i].top );
		file->WriteInt( outFont->glyphs[i].bottom );
		file->WriteInt( outFont->glyphs[i].pitch );
		file->WriteInt( outFont->glyphs[i].xSkip );
		file->WriteInt( outFont->glyphs[i].imageWidth );
		file->WriteInt( outFont->glyphs[i].imageHeight );
		file->WriteFloat( outFont->glyphs[i].s );
		file->WriteFloat( outFont->glyphs[i].t );
		file->WriteFloat( outFont->glyphs[i].s2 );
		file->WriteFloat( outFont->glyphs[i].t2 );
		// write junk, formerly font.glyphs[i].glyph
		file->WriteInt( 0 );

		for (int j = 0; j < 32; j++) {
			file->WriteChar( outFont->glyphs[i].shaderName[j] );
		}
	}
	file->WriteFloat( outFont->glyphScale );

	fileSystem->CloseFile( file );

	return true;
}

/*
============
RegisterFont

Loads 3 point sizes, 12, 24, and 48
============
*/
bool idRenderSystemLocal::RegisterFont( const char *fontName, fontInfoEx_t &font ) {
#ifdef ID_BUILD_FREETYPE
	FT_Face face;
	int j, k, xOut, yOut, lastStart, imageNumber;
	int scaledSize, newSize, maxHeight, left;
	unsigned char *out, *imageBuff;
	glyphInfo_t *glyph;
	const idMaterial *h;
	float max;
	void *faceData;
	ID_TIME_T ftime;
	int i, len, fontCount;
#else
	int fontCount;
#endif // ID_BUILD_FREETYPE
	idStr name;
	float dpi = 72;
	int pointSize = 12;

	memset( &font, 0, sizeof( font ) );

	bool allPointSizesLoaded = true;
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

		sprintf( name, "%s/fontImage_%i.dat", fontName, pointSize );

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

		idStr::Copynz( outFont->name, name.c_str(), sizeof( outFont->name ) );

		if ( !LoadFontInfo( name.c_str(), outFont ) ) {
			allPointSizesLoaded = false;
			break;
		}

		FinalizeFontInfoEx( fontName, font, outFont, fontCount );
	}

	if ( allPointSizesLoaded ) {
		return true ;
	}

#ifndef ID_BUILD_FREETYPE
	common->Warning( "RegisterFont: couldn't load FreeType code %s", name.c_str() );
	return false;
#else

	if (ftLibrary == NULL) {
		common->Warning( "RegisterFont: FreeType not initialized." );
		return false;
	}

	name = fontName;
	name.StripFileExtension();
	name.SetFileExtension( ".ttf" );
	len = fileSystem->ReadFile( name, &faceData, &ftime );
	if ( len <= 0 ) {
		common->Warning( "RegisterFont: Unable to read font file: '%s'", name.c_str() );
		return false;
	}

	// make a 256x256 image buffer, once it is full, register it, clean it and keep going
	// until all glyphs are rendered

	out = ( byte* ) Mem_Alloc( 1024 * 1024 );
	if ( out == NULL ) {
		common->Warning( "RegisterFont: Mem_Alloc failure during output image creation." );
		return false;
	}

	for ( fontCount = 0; fontCount < 3; fontCount++ ) {
		if ( fontCount == 0 ) {
			pointSize = 12;
		} else if( fontCount == 1 ) {
			pointSize = 24;
		} else {
			pointSize = 48;
		}

		// allocate on the stack first in case we fail
		if ( FT_New_Memory_Face( ftLibrary, ( const FT_Byte* ) faceData, len, 0, &face ) ) {
			common->Warning( "RegisterFont: FreeType2, unable to allocate new face." );
			return false;
		}

		if ( FT_Set_Char_Size( face, pointSize << 6, pointSize << 6, dpi, dpi) ) {
			common->Warning( "RegisterFont: FreeType2, Unable to set face char size." );
			return false;
		}

		sprintf( name, "%s/fontImage_%i.dat", fontName, pointSize );

		fontInfo_t* outFont;
		if ( fontCount == 0 ) {
			outFont = &font.fontInfoSmall;
		} else if ( fontCount == 1 ) {
			outFont = &font.fontInfoMedium;
		} else {
			outFont = &font.fontInfoLarge;
		}

		// we also need to adjust the scale based on point size relative to 48 points as the ui scaling is based on a 48 point font
		float glyphScale = 1.0f; 		// change the scale to be relative to 1 based on 72 dpi ( so dpi of 144 means a scale of .5 )
		glyphScale *= 48.0f / pointSize;
		outFont->glyphScale = glyphScale;

		idStr::Copynz( outFont->name, name.c_str(), sizeof( outFont->name ) );

		memset( out, 0, 1024 * 1024 );

		// calculate max height
		maxHeight = 0;
		for ( i = GLYPH_START; i < GLYPH_END; i++ ) {
			glyph = RE_ConstructGlyphInfo( out, &xOut, &yOut, &maxHeight, face, ( unsigned char )i, true );
		}

		xOut = 0;
		yOut = 0;
		i = GLYPH_START;
		lastStart = i;
		imageNumber = 0;

		while ( i <= GLYPH_END ) {
			glyph = RE_ConstructGlyphInfo(out, &xOut, &yOut, &maxHeight, face, (unsigned char)i, false);

			if (xOut == -1 || yOut == -1 || i == GLYPH_END)  {
				// ran out of room
				// we need to create an image from the bitmap, set all the handles in the glyphs to this point
				//

				scaledSize = FONT_SIZE*FONT_SIZE;
				newSize = scaledSize * 4;
				imageBuff = (byte*)Mem_Alloc(newSize);
				left = 0;
				max = 0;
				//satLevels = 255;
				for ( k = 0; k < (scaledSize) ; k++ ) {
					if (max < out[k]) {
						max = out[k];
					}
				}

				if (max > 0) {
					max = 255/max;
				}

				for ( k = 0; k < (scaledSize) ; k++ ) {
					imageBuff[left++] = 255;
					imageBuff[left++] = 255;
					imageBuff[left++] = 255;
					imageBuff[left++] = ((float)out[k] * max);
				}

#ifdef ID_FREETYPE_PNG
				sprintf( name, "%s/fontImage_%i_%i.png", fontName, imageNumber, pointSize );
				R_WritePNG( name.c_str(), imageBuff, 4, FONT_SIZE, FONT_SIZE, true, "fs_basepath" );
#else
				sprintf( name, "%s/fontImage_%i_%i.tga", fontName, imageNumber, pointSize );
				R_WriteTGA( name.c_str(), imageBuff, FONT_SIZE, FONT_SIZE, false, "fs_basepath" );
#endif // ID_BUILD_FREETYPE

				h = declManager->FindMaterial( name );
				h->SetSort( SS_GUI );

				memset( out, 0, 1024*1024 );
				xOut = 0;
				yOut = 0;
				Mem_Free( imageBuff );

				//i++;

#ifdef ID_FREETYPE_PNG
				sprintf( name, "fonts/fontImage_%i_%i.png", imageNumber, pointSize );
#else
				sprintf( name, "fonts/fontImage_%i_%i.tga", imageNumber, pointSize );
#endif // ID_BUILD_FREETYPE

				imageNumber++;

				if ( i == GLYPH_END ) {
					for ( j = lastStart; j <= GLYPH_END; j++ ) {
						outFont->glyphs[j].glyph = h;
						idStr::Copynz( outFont->glyphs[j].shaderName, name.c_str(), sizeof( outFont->glyphs[j].shaderName ) );
					}
					break;
				} else {
					for ( j = lastStart; j < i; j++ ) {
						outFont->glyphs[j].glyph = h;
						idStr::Copynz( outFont->glyphs[j].shaderName, name.c_str(), sizeof( outFont->glyphs[j].shaderName ) );
					}
					lastStart = i;
				}
			} else {
				memcpy( &outFont->glyphs[i], glyph, sizeof( glyphInfo_t ) );
				i++;
			}
		}

		SaveFontInfo( va( "%s/fontImage_%i.dat", fontName, pointSize ), outFont );

		FinalizeFontInfoEx( fontName, font, outFont, fontCount );
	}

	Mem_Free( out );

	fileSystem->FreeFile( faceData );
#endif // ID_BUILD_FREETYPE
	return true;
}

/*
============
R_InitFreeType
============
*/
void R_InitFreeType( void ) {
#ifdef ID_BUILD_FREETYPE
	if ( FT_Init_FreeType( &ftLibrary ) ) {
		common->Printf( "R_InitFreeType: Unable to initialize FreeType.\n" );
	}
#endif // ID_BUILD_FREETYPE
//	registeredFontCount = 0;
}

/*
============
R_DoneFreeType
============
*/
void R_DoneFreeType( void ) {
#ifdef ID_BUILD_FREETYPE
	if ( ftLibrary ) {
		FT_Done_FreeType( ftLibrary );
		ftLibrary = NULL;
	}
#endif // ID_BUILD_FREETYPE
//	registeredFontCount = 0;
}
