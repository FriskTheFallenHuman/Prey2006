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
idMipMap::CreateMips
===================
*/
void idMipMap::CreateMips( uint8_t *data, const uint8_t power ) {
	uint8_t *currentMip = data;
	int mipSize = 1 << power;
	int numMips = power - 1;
	const uint8_t *sourceMip = data;
	uint8_t *nextMip = &data[4*mipSize*mipSize];
	uint8_t *destinationMip = nextMip;

	for ( int currentMipLevel = numMips; currentMipLevel >= 2; --currentMipLevel ) {
		mipSize >>= 1;
		uint8_t *halfMip = &currentMip[8*mipSize];
		const uint8_t *inPic = halfMip;
		int currentMipSize = mipSize;
		int halfMipSize = (mipSize - 1) / 2 + 1;
		int mipBytes = 16 * mipSize;

		for ( int i = mipSize; i > 0; --i ) {
			uint8_t *out = nextMip + 2;
			const uint8_t *in1 = halfMip + 1;
			const uint8_t *in2 = currentMip + 4;
			int offset = halfMip - currentMip;

			for ( unsigned int j = halfMipSize; j > 0; --j ) {
				*(out - 2) = (*in2 + *(in1 - 1) + *(in2 - 4) + in2[offset]) >> 2;
				*(out - 1) = (*in1 + in1[4] + in2[1] + *(in2 - 3)) >> 2;
				*out = (in1[1] + in1[5] + in2[2] + *(in2 - 2)) >> 2;
				out[1] = (in1[2] + in1[6] + *(in2 - 1) + in2[3]) >> 2;
				out[2] = (in1[7] + in1[11] + in2[4] + in2[8]) >> 2;
				out[3] = (in1[8] + in1[12] + in2[5] + in2[9]) >> 2;
				out[4] = (in1[9] + in1[13] + in2[6] + in2[10]) >> 2;
				out[5] = (in1[10] + in1[14] + in2[7] + in2[11]) >> 2;

				in1 += 16;
				in2 += 16;
				out += 8;
			}

			currentMip = (uint8_t*)inPic + mipBytes;
			halfMip = (uint8_t*)halfMip + mipBytes;
			nextMip = destinationMip + 4 * currentMipSize;
			destinationMip = nextMip;
		}
	}
}