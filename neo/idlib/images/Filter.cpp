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

/*
===================
GetCubicWeights
===================
*/
void GetCubicWeights( float s, float A, float *bvals ) {
	double sA = s * A;
	double sAs = s * sA;
	double sAss = s * sAs;

	bvals[0] = sA - 2 * A * s * s + sAss;
	bvals[1] = 1.0f - (A + 3.0f) * s * s + (A + 2.0f) * s * s * s;
	bvals[2] = s * (2 * A + 3.0f) * s - sA - (A + 2.0f) * s * s * s;
	bvals[3] = sAs - sAss;
}

/*
===================
idFilter::UpScale2xBicubic_Generic
===================
*/
void idFilter::UpScale2xBicubic_Generic( const uint8_t *src, int width, int height, int stride, uint8_t *dst, bicubicFilter_t type ) {
	float A = -0.75f;
	float bvals[4];
	float weights[4];
	float table[4][16] = { 0 };

	if ( type == BICUBIC_SHIFTED ) {
		GetCubicWeights( 0.8f, A, bvals);
		GetCubicWeights( 0.2f, A, weights );
	} else {
		GetCubicWeights( 0.75f, A, bvals );
		GetCubicWeights( 0.25f, A, weights );
	}

	for ( int i = 0; i < 2; ++i ) {
		int index = i & 1;
		for ( int j = 0; j < 2; ++j ) {
			int offset = j & 1;
			float *sourceWeights = &bvals[4 * offset];
			float *tablePtr = &table[offset | (2 * index)][2];
			for ( int k = 0; k < 4; ++k ) {
				tablePtr[0] = sourceWeights[0] * weights[k];
				tablePtr[1] = sourceWeights[1] * weights[k];
				tablePtr[2] = sourceWeights[2] * weights[k];
				tablePtr[3] = sourceWeights[3] * weights[k];
				tablePtr += 4;
			}
		}
	}

	for ( int y = 0; y < 2 * height; ++y ) {
		for ( int x = 0; x < 2 * width; ++x ) {
			int srcY = (y >> 1) - 1;
			int srcX = (x >> 1) - 1;

			float accum[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

			for ( int ky = 0; ky < 4; ++ky ) {
				int yy = srcY + ky;
				yy = idMath::ClampInt( yy, 0, height - 1 );

				for (int kx = 0; kx < 4; ++kx) {
					int xx = srcX + kx;
					xx = idMath::ClampInt( xx, 0, width - 1 );

					for (int c = 0; c < 4; ++c) {
						accum[c] += src[4 * (yy * stride + xx) + c] * table[ky][kx * 4 + c];
					}
				}
			}

			for ( int c = 0; c < 4; ++c ) {
				dst[4 * ( y * 2 * width + x ) + c] = static_cast<uint8_t>( idMath::ClampInt( accum[c], 0.0f, 255.0f ) );
			}
		}
	}
}
