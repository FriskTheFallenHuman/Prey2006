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

#define STBI_NO_HDR
#define STBI_NO_LINEAR
#define STBI_NO_STDIO  // images are passed as buffers
#include "stb/stb_image.h"

#include "stb/stb_image_write.h"

typedef struct {
	const char*	ext;
	void	( *ImageLoader )( const char *filename, unsigned char **pic, int *width, int *height, ID_TIME_T *timestamp );
} imageExtToLoader_t;

static imageExtToLoader_t imageLoaders[] = {
	{"tga", RB_LoadImage},
	{"bmp", RB_LoadImage},
	{"png", RB_LoadImage},
	{"jpg", RB_LoadImage},
};

static const int numImageLoaders = sizeof(imageLoaders) / sizeof(imageLoaders[0]);

/*
================
R_WriteImage
================
*/
void R_WriteImage( idImageType filetype, const char *filename, const byte *data, int bytesPerPixel, int width, int height, bool flipVertical, const char *basePath ) {
	if ( bytesPerPixel != 4  && bytesPerPixel != 3 ) {
		common->Error( "R_WriteImage( %s ): bytesPerPixel = %i not supported", filename, bytesPerPixel );
	}

	idFileLocal file( fileSystem->OpenFileWrite( filename, basePath ) );
	if ( file == NULL ) {
		common->Printf( "R_WriteImage: Failed to open %s\n", filename );
		return;
	}

	stbi_flip_vertically_on_write( flipVertical );

	switch ( filetype ) {
		default:
		case TYPE_TGA:
			stbi_write_tga_to_func( WriteScreenshotForSTBIW, file, width, height, bytesPerPixel, data );
			break;
		case TYPE_BMP:
			stbi_write_bmp_to_func( WriteScreenshotForSTBIW, file, width, height, bytesPerPixel, data );
			break;
		case TYPE_PNG:
			//if ( r_screenshotQuality.GetInteger() > 9 ) {
				// Since we use this cvar for jpeg quality, reset the cvar back at default values
			//	r_screenshotQuality.SetInteger( 3 );
			//}
			//stbi_write_png_compression_level = idMath::ClampInt( 0, 9, r_screenshotQuality.GetInteger() );
			stbi_write_png_to_func( WriteScreenshotForSTBIW, file, width, height, bytesPerPixel, data, bytesPerPixel * width );
			break;
		case TYPE_JPEG:
			stbi_write_jpg_to_func( WriteScreenshotForSTBIW, file, width, height, bytesPerPixel, data, 100 /*idMath::ClampInt( 1, 100, r_screenshotQuality.GetInteger() )*/ );
			break;
	}
}

/*
=========================================================

PNG/TGA/BMP/JPG LOADING

Interfaces with the huge stb
=========================================================
*/

/*
=============
RB_LoadImage
=============
*/
void RB_LoadImage( const char *filename, unsigned char **pic, int *width, int *height, ID_TIME_T *timestamp ) {
	if ( pic ) {
		*pic = NULL;		// until proven otherwise
	}

	idFile* f = fileSystem->OpenFileRead( filename );
	if ( !f ) {
		return;
	}
	int len = f->Length();
	if ( timestamp ) {
		*timestamp = f->Timestamp();
	}
	if ( !pic ) {
		fileSystem->CloseFile( f );
		return;	// just getting timestamp
	}
	byte* fbuffer = ( byte* )Mem_ClearedAlloc( len, TAG_IMAGE );
	f->Read( fbuffer, len );
	fileSystem->CloseFile( f );

	int w = 0, h = 0, comp = 0;
	byte *decodedImageData = stbi_load_from_memory( fbuffer, len, &w, &h, &comp, 4 );

	Mem_Free( fbuffer );

	if ( decodedImageData == NULL ) {
		common->Warning( "stb_image was unable to load image %s : %s\n", filename, stbi_failure_reason() );
		return;
	}

	// *pic must be allocated with R_StaticAlloc(), but stb_image allocates with malloc()
	// (and as there is no R_StaticRealloc(), #define STBI_MALLOC etc won't help)
	// so the decoded data must be copied once
	int size = w*h*4;
	*pic = (byte *)R_StaticAlloc( size );
	memcpy( *pic, decodedImageData, size );
	*width = w;
	*height = h;
	// now that decodedImageData has been copied into *pic, it's not needed anymore
	stbi_image_free( decodedImageData );
}

//===================================================================

/*
=================
R_LoadImage

Loads any of the supported image types into a cannonical
32 bit format.

Automatically attempts to load .jpg files if .tga files fail to load.

*pic will be NULL if the load failed.

Anything that is going to make this into a texture would use
makePowerOf2 = true, but something loading an image as a lookup
table of some sort would leave it in identity form.

It is important to do this at image load time instead of texture load
time for bump maps.

Timestamp may be NULL if the value is going to be ignored

If pic is NULL, the image won't actually be loaded, it will just find the
timestamp.
=================
*/
void R_LoadImage( const char *cname, byte **pic, int *width, int *height, ID_TIME_T *timestamp, bool makePowerOf2 ) {
	idStr name = cname;

	if ( pic ) {
		*pic = NULL;
	}
	if ( timestamp ) {
		*timestamp = FILE_NOT_FOUND_TIMESTAMP;
	}
	if ( width ) {
		*width = 0;
	}
	if ( height ) {
		*height = 0;
	}

	name.DefaultFileExtension( ".tga" );

	if (name.Length()<5) {
		return;
	}

	name.ToLower();
	idStr ext;
	name.ExtractFileExtension( ext );

	// try
	if ( !ext.IsEmpty() ) {
		// try only the image with the specified extension: default .tga
		int i;
		for ( i = 0; i < numImageLoaders; i++ ) {
			if( !ext.Icmp( imageLoaders[i].ext ) )
			{
				imageLoaders[i].ImageLoader( name.c_str(), pic, width, height, timestamp );
				break;
			}
		}

		if ( i < numImageLoaders ) {
			if ( ( pic && *pic == NULL ) || ( timestamp && *timestamp == FILE_NOT_FOUND_TIMESTAMP ) ) {
				// image with the specified extension was not found so try all extensions
				for ( i = 0; i < numImageLoaders; i++ ) {
					name.SetFileExtension( imageLoaders[i].ext );
					imageLoaders[i].ImageLoader( name.c_str(), pic, width, height, timestamp );

					if ( pic && *pic != NULL ) {
						break;
					}

					if ( !pic && timestamp && *timestamp != FILE_NOT_FOUND_TIMESTAMP ) {
						break;
					}
				}
			}
		}
	}

	if ( ( width && *width < 1 ) || ( height && *height < 1 ) ) {
		if ( pic && *pic ) {
			R_StaticFree( *pic );
			*pic = 0;
		}
	}

	//
	// convert to exact power of 2 sizes
	//
	/*
	if ( pic && *pic && makePowerOf2 ) {
		int		w, h;
		int		scaled_width, scaled_height;
		byte	*resampledBuffer;

		w = *width;
		h = *height;

		for (scaled_width = 1 ; scaled_width < w ; scaled_width<<=1)
			;
		for (scaled_height = 1 ; scaled_height < h ; scaled_height<<=1)
			;

		if ( scaled_width != w || scaled_height != h ) {
			resampledBuffer = R_ResampleTexture( *pic, w, h, scaled_width, scaled_height );
			R_StaticFree( *pic );
			*pic = resampledBuffer;
			*width = scaled_width;
			*height = scaled_height;
		}
	}
	*/
}


/*
=======================
R_LoadCubeImages

Loads six files with proper extensions
=======================
*/
bool R_LoadCubeImages( const char *imgName, cubeFiles_t extensions, byte *pics[6], int *outSize, ID_TIME_T *timestamp ) {
	int		i, j;
	char	*cameraSides[6] =  { "_forward.tga", "_back.tga", "_left.tga", "_right.tga",
		"_up.tga", "_down.tga" };
	char	*axisSides[6] =  { "_px.tga", "_nx.tga", "_py.tga", "_ny.tga",
		"_pz.tga", "_nz.tga" };
	char	**sides;
	char	fullName[MAX_IMAGE_NAME];
	int		width, height, size = 0;

	if ( extensions == CF_CAMERA ) {
		sides = cameraSides;
	} else {
		sides = axisSides;
	}

	// FIXME: precompressed cube map files
	if ( pics ) {
		memset( pics, 0, 6*sizeof(pics[0]) );
	}
	if ( timestamp ) {
		*timestamp = 0;
	}

	for ( i = 0 ; i < 6 ; i++ ) {
		idStr::snPrintf( fullName, sizeof( fullName ), "%s%s", imgName, sides[i] );

		ID_TIME_T thisTime;
		if ( !pics ) {
			// just checking timestamps
			R_LoadImageProgram( fullName, NULL, &width, &height, &thisTime );
		} else {
			R_LoadImageProgram( fullName, &pics[i], &width, &height, &thisTime );
		}
		if ( thisTime == FILE_NOT_FOUND_TIMESTAMP ) {
			break;
		}
		if ( i == 0 ) {
			size = width;
		}
		if ( width != size || height != size ) {
			common->Warning( "Mismatched sizes on cube map '%s'", imgName );
			break;
		}
		if ( timestamp ) {
			if ( thisTime > *timestamp ) {
				*timestamp = thisTime;
			}
		}
		if ( pics && extensions == CF_CAMERA ) {
			// convert from "camera" images to native cube map images
			switch( i ) {
			case 0:	// forward
				R_RotatePic( pics[i], width);
				break;
			case 1:	// back
				R_RotatePic( pics[i], width);
				R_HorizontalFlip( pics[i], width, height );
				R_VerticalFlip( pics[i], width, height );
				break;
			case 2:	// left
				R_VerticalFlip( pics[i], width, height );
				break;
			case 3:	// right
				R_HorizontalFlip( pics[i], width, height );
				break;
			case 4:	// up
				R_RotatePic( pics[i], width);
				break;
			case 5: // down
				R_RotatePic( pics[i], width);
				break;
			}
		}
	}

	if ( i != 6 ) {
		// we had an error, so free everything
		if ( pics ) {
			for ( j = 0 ; j < i ; j++ ) {
				R_StaticFree( pics[j] );
			}
		}

		if ( timestamp ) {
			*timestamp = 0;
		}
		return false;
	}

	if ( outSize ) {
		*outSize = size;
	}
	return true;
}
