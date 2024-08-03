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

#include "Decoder_local.h"

/*
===================
idBareDCTHuffmanTable::Init
===================
*/
void idBareDCTHuffmanTable::Init( const uint8_t *pBits, const uint8_t *pVal ) {
	memcpy(this->bits, pBits, sizeof(this->bits));
	memcpy(this->symbols, pVal, sizeof(this->symbols));

	char huffsize[257] = { 0 };
	unsigned int huffcode[257] = { 0 };

	int position = 0;
	for (int i = 1; i <= 16; ++i) {
		int count = pBits[i];
		if (count) {
			memset(&huffsize[position + 1], i, count);
			position += count;
		}
	}
	huffsize[position + 1] = 0;

	unsigned int code = 0;
	int last_length = huffsize[1];
	for (int i = 1; huffsize[i]; ++i) {
		if (huffsize[i] == last_length) {
			huffcode[i] = code++;
		} else {
			code <<= (huffsize[i] - last_length);
			huffcode[i] = code++;
			last_length = huffsize[i];
		}
	}

	int j = 0;
	for (int i = 1; i <= 16; ++i) {
		if (pBits[i]) {
			minCode[i] = huffcode[j + 1];
			symOffset[i] = j;
			j += pBits[i];
			maxCode[i] = huffcode[j];
		} else {
			minCode[i] = maxCode[i] = symOffset[i] = -1;
		}
	}
	maxCode[17] = 0xFFFFF;

	memset(this->look_nbits, 0, sizeof(this->look_nbits));
	memset(this->look_sym, 0, sizeof(this->look_sym));

	int pIndex = 0;
	for (int length = 1; length <= 8; ++length) {
		if (pBits[length]) {
			for (int i = 0; i < (1 << (8 - length)); ++i) {
				int code = huffcode[pIndex + 1] << (8 - length) | i;
				this->look_nbits[code] = length;
				this->look_sym[code] = pVal[pIndex];
			}
			pIndex += pBits[length];
		}
	}

	memset(this->test_nbits, 0, sizeof(this->test_nbits));
	pIndex = 0;
	for (int i = 1; i <= 16; ++i) {
		if (pBits[i]) {
			for (int j = 0; j < pBits[i]; ++j) {
				if (i < 16) {
					int value = (huffcode[pIndex + 1] + 1) << (16 - i);
					std::fill(&this->test_nbits[i], &this->test_nbits[16], value);
				}
				++pIndex;
			}
		}
	}

	memset(this->code, 0, sizeof(this->code));
	memset(this->size, 0, sizeof(this->size));

	for (int i = 0; i < position; ++i) {
		this->code[pVal[i]] = huffcode[i + 1];
		this->size[pVal[i]] = huffsize[i + 1];
	}
}

/*
===================
idBareDctDecoder::DecodeLong_MMX
===================
*/
int idBareDctDecoder::DecodeLong_MMX( const idBareDCTHuffmanTable *htbl ) {
	if (getBits < 16) {
		dataBytes -= 2;
		bool dataAvailable = dataBytes >= 0;
		getBuff = data[dataAvailable] | ((data[0] | (getBuff << 8)) << 8);
		getBits += 16;
		data += 2 * dataAvailable;
	}

	int currentBits = getBits;
	int currentCode = static_cast<uint16_t>(getBuff >> (currentBits - 16));

	int length = (currentCode >= htbl->test_nbits[15]) +
				 (currentCode >= htbl->test_nbits[14]) +
				 (currentCode >= htbl->test_nbits[13]) +
				 (currentCode >= htbl->test_nbits[12]) +
				 (currentCode >= htbl->test_nbits[11]) +
				 (currentCode >= htbl->test_nbits[10]) +
				 (currentCode >= htbl->test_nbits[9]) + 9;

	getBits -= length;
	int symbolIndex = htbl->symOffset[length] +
					  (currentCode >> (16 - length)) -
					  htbl->minCode[length];

	return htbl->symbols[symbolIndex];
}

/*
===================
idBareDctDecoder::HuffmanDecode
===================
*/
void idBareDctDecoder::HuffmanDecode( int16_t *coef, const idBareDCTHuffmanTable *dctbl, const idBareDCTHuffmanTable *actbl, int *lastDC ) {
	memset(coef, 0, 128);  // 0x80u is 128 bytes

	// Decode DC coefficient
	if (getBits < 8) {
		dataBytes -= 2;
		bool dataAvailable = dataBytes >= 0;
		getBuff = data[dataAvailable] | ((data[0] | (getBuff << 8)) << 8);
		getBits += 16;
		data += 2 * dataAvailable;
	}

	int dcValue;
	int code = getBuff >> (getBits - 8);
	if (dctbl->look_nbits[code]) {
		getBits -= dctbl->look_nbits[code];
		dcValue = dctbl->look_sym[code];
	} else {
		dcValue = DecodeLong_MMX(dctbl);
	}

	if (dcValue) {
		if (getBits < dcValue) {
			dataBytes -= 2;
			bool dataAvailable = dataBytes >= 0;
			getBuff = data[dataAvailable] | ((data[0] | (getBuff << 8)) << 8);
			getBits += 16;
			data += 2 * dataAvailable;
		}
		getBits -= dcValue;
		int value = ((1 << dcValue) - 1) & (getBuff >> getBits);
		dcValue = ((1 - (1 << dcValue)) & ((value - (1 << (dcValue - 1))) >> 31)) + value;
	}

	*lastDC += dcValue;
	coef[0] = *lastDC;

	// Decode AC coefficients
	const idBareDCTHuffmanTable* currentTable = actbl;
	for (int i = 1; i < 64; ) {
		if (getBits < 8) {
			dataBytes -= 2;
			bool dataAvailable = dataBytes >= 0;
			getBuff = data[dataAvailable] | ((data[0] | (getBuff << 8)) << 8);
			getBits += 16;
			data += 2 * dataAvailable;
		}

		code = getBuff >> (getBits - 8);
		int acValue;
		if (currentTable->look_nbits[code]) {
			getBits -= currentTable->look_nbits[code];
			acValue = currentTable->look_sym[code];
		} else {
			acValue = DecodeLong_MMX(currentTable);
		}

		int runLength = acValue >> 4;
		int numBits = acValue & 0xF;
		i += runLength;

		if (numBits) {
			if (getBits < numBits) {
				dataBytes -= 2;
				bool dataAvailable = dataBytes >= 0;
				getBuff = data[dataAvailable] | ((data[0] | (getBuff << 8)) << 8);
				getBits += 16;
				data += 2 * dataAvailable;
			}
			getBits -= numBits;
			int value = ((1 << numBits) - 1) & (getBuff >> getBits);
			coef[jpeg_natural_order[i]] = value + ((1 - (1 << numBits)) & ((value - (1 << (numBits - 1))) >> 31));
		} else if (runLength != 15) {
			return;
		}

		i++;
	}
}

/*
===================
idBareDctBase::InitHuffmanTable
===================
*/
void idBareDctBase::InitHuffmanTable( void ) {
	huffTableYDC.Init(bitsYDC_0, valYDC_0);
	huffTableYAC.Init(bitsYAC_0, valYAC_0);
	huffTableCoCgDC.Init(bitsCoCgDC_0, valCoCgDC_0);
	huffTableCoCgAC.Init(bitsCoCgAC_0, valCoCgAC_0);
	huffTableADC.Init(bitsYDC_0, valYDC_0);
	huffTableAAC.Init(bitsYAC_0, valYAC_0);
}

/*
===================
idBareDctBase::InitQuantTable
===================
*/
void idBareDctBase::InitQuantTable( void ) {
	quantTableY = reinterpret_cast<uint16_t*>(Mem_Alloc16(0x80));
	quantTableCoCg = reinterpret_cast<uint16_t*>(Mem_Alloc16(0x80));
	quantTableA = reinterpret_cast<uint16_t*>(Mem_Alloc16(0x80));

	SetQuality_Generic(luminanceQuality, chrominanceQuality, alphaQuality);
}

/*
===================
idBareDctBase::QuantizationScaleFromQuality
===================
*/
int idBareDctBase::QuantizationScaleFromQuality( int quality ) {
	int v2 = quality;

	if (quality <= 0) {
		return 5000;
	}
	if (quality > 100) {
		v2 = 100;
		return 2 * (100 - v2);
	}
	if (quality >= 50) {
		return 2 * (100 - v2);
	}
	return 5000 / quality;
}

/*
===================
idBareDctBase::ScaleQuantTable
===================
*/
void idBareDctBase::ScaleQuantTable( uint16_t *result, const uint16_t* standard, int scale ) {
	for (int i = 0; i < 64; ++i) {
		int value = (scale * 256 / 100) * standard[i] / 65536;
		if (value < 1) {
			value = 1;
		}
		else if (value > 255) {
			value = 255;
		}
		result[i] = static_cast<uint16_t>(value);
	}
}

/*
===================
idBareDctBase::SetQuality_Generic
===================
*/
void idBareDctBase::SetQuality_Generic( int luminanceQuality, int chrominanceQuality, int alphaQuality ) {
	int luminanceScale, chrominanceScale, alphaScale;

	if (luminanceQuality <= 0) {
		luminanceScale = 5000;
	} else if (luminanceQuality > 100) {
		luminanceScale = 2 * (100 - 100);
	} else if (luminanceQuality >= 50) {
		luminanceScale = 2 * (100 - luminanceQuality);
	} else {
		luminanceScale = 5000 / luminanceQuality;
	}

	if (chrominanceQuality <= 0) {
		chrominanceScale = 5000;
	} else if (chrominanceQuality > 100) {
		chrominanceScale = 2 * (100 - 100);
	} else if (chrominanceQuality >= 50) {
		chrominanceScale = 2 * (100 - chrominanceQuality);
	} else {
		chrominanceScale = 5000 / chrominanceQuality;
	}

	if (alphaQuality <= 0) {
		alphaScale = 5000;
	} else if (alphaQuality > 100) {
		alphaScale = 2 * (100 - 100);
	} else if (alphaQuality >= 50) {
		alphaScale = 2 * (100 - alphaQuality);
	} else {
		alphaScale = 5000 / alphaQuality;
	}

	ScaleQuantTable(quantTableY, std_luminance_quant_tbl_0, luminanceScale);
	ScaleQuantTable(quantTableCoCg, std_chrominance_quant_tbl_0, chrominanceScale);
	ScaleQuantTable(quantTableA, std_alpha_quant_tbl_0, alphaScale);
}

/*
===================
idBareDctDecoder::DecompressImageRGBA_Generic
===================
*/
void idBareDctDecoder::DecompressImageRGBA_Generic( const uint8_t *inBuf, uint8_t *outBuf, int width, int height, int inputBytes ) {
	this->data = inBuf;
	this->imageWidth = width;
	this->imageHeight = height;
	this->dataBytes = inputBytes;
	this->getBits = 0;
	this->getBuff = 0;
	this->dcY = 0;
	this->dcCo = 0;
	this->dcCg = 0;
	this->dcA = 0;

	int numVerticalTiles = (height + 15) >> 4;
	int numHorizontalTiles = (width + 15) >> 4;

	for (int tileY = 0; tileY < numVerticalTiles; ++tileY) {
		for (int tileX = 0; tileX < numHorizontalTiles; ++tileX) {
			uint8_t Src[1024];
			DecompressOneTileRGBA(Src, 64);

			int destY = 16 * tileY;
			int destX = 16 * tileX;

			int copyHeight = (destY + 16 > height) ? height - destY : 16;
			int copyWidth = (destX + 16 > width) ? width - destX : 16;

			uint8_t *dest = &outBuf[4 * width * destY + 4 * destX];

			for (int row = 0; row < copyHeight; ++row) {
				memcpy(dest, Src + row * 64, 4 * copyWidth);
				dest += 4 * width;
			}
		}
	}
}

/*
===================
idBareDctDecoder::DecompressImageRGB_Generic
===================
*/
void idBareDctDecoder::DecompressImageRGB_Generic( const uint8_t *inBuf, uint8_t *outBuf, int width, int height, int inputBytes ) {
	this->data = inBuf;
	this->imageWidth = width;
	this->imageHeight = height;
	this->dataBytes = inputBytes;
	this->getBits = 0;
	this->getBuff = 0;
	this->dcY = 0;
	this->dcCo = 0;
	this->dcCg = 0;
	this->dcA = 0;

	int numVerticalTiles = (height + 15) >> 4;
	int numHorizontalTiles = (width + 15) >> 4;

	for (int tileY = 0; tileY < numVerticalTiles; ++tileY) {
		for (int tileX = 0; tileX < numHorizontalTiles; ++tileX) {
			uint8_t Src[1024];
			DecompressOneTileRGB(Src, 64);

			int destY = 16 * tileY;
			int destX = 16 * tileX;

			int copyHeight = (destY + 16 > height) ? height - destY : 16;
			int copyWidth = (destX + 16 > width) ? width - destX : 16;

			uint8_t *dest = &outBuf[3 * width * destY + 3 * destX];

			for (int row = 0; row < copyHeight; ++row) {
				memcpy(dest, Src + row * 48, 3 * copyWidth);
				dest += 3 * width;
			}
		}
	}
}

/*
===================
idBareDctDecoder::DecompressImageYCoCg_Generic
===================
*/
void idBareDctDecoder::DecompressImageYCoCg_Generic( const uint8_t *inBuf, uint8_t *outBuf, int width, int height, int inputBytes ) {
	this->data = inBuf;
	this->imageWidth = width;
	this->imageHeight = height;
	this->dataBytes = inputBytes;
	this->getBits = 0;
	this->getBuff = 0;
	this->dcY = 0;
	this->dcCo = 0;
	this->dcCg = 0;
	this->dcA = 0;

	int numVerticalTiles = (height + 15) >> 4;
	int numHorizontalTiles = (width + 15) >> 4;

	for (int tileY = 0; tileY < numVerticalTiles; ++tileY) {
		for (int tileX = 0; tileX < numHorizontalTiles; ++tileX) {
			uint8_t rgb[1024];
			DecompressOneTileYCoCg(rgb, 64);

			int destY = 16 * tileY;
			int destX = 16 * tileX;

			int copyHeight = (destY + 16 > height) ? height - destY : 16;
			int copyWidth = (destX + 16 > width) ? width - destX : 16;

			uint8_t *dest = &outBuf[4 * width * destY + 4 * destX];

			for (int row = 0; row < copyHeight; ++row) {
				memcpy(dest, rgb + row * 64, 4 * copyWidth);
				dest += 4 * width;
			}
		}
	}
}

/*
===================
idBareDctDecoder::DecompressLuminanceEnhancement_Generic
===================
*/
void idBareDctDecoder::DecompressLuminanceEnhancement_Generic( const uint8_t *inBuf, uint8_t *outBuf, int width, int height, int inputBytes ) {
	this->data = inBuf;
	this->imageWidth = width;
	this->imageHeight = height;
	this->dataBytes = inputBytes;
	this->getBits = 0;
	this->getBuff = 0;
	this->dcY = 0;
	this->dcCo = 0;
	this->dcCg = 0;
	this->dcA = 0;

	int numVerticalTiles = (height + 7) >> 3;
	int numHorizontalTiles = (width + 7) >> 3;

	for (int tileY = 0; tileY < numVerticalTiles; ++tileY) {
		for (int tileX = 0; tileX < numHorizontalTiles; ++tileX) {
			uint8_t Src[256];

			// Calculate the dimensions of the current tile
			int tileWidth = (tileX + 1) * 8 > width ? width - tileX * 8 : 8;
			int tileHeight = (tileY + 1) * 8 > height ? height - tileY * 8 : 8;

			// Copy the current tile from the output buffer to the temporary buffer
			uint8_t *srcPtr = Src;
			uint8_t *destPtr = &outBuf[(tileY * 8 * width + tileX * 8) * 4];
			for (int row = 0; row < tileHeight; ++row) {
				memcpy(srcPtr, destPtr, tileWidth * 4);
				srcPtr += 32; // 8 * 4
				destPtr += width * 4;
			}

			// Decompress the current tile
			DecompressOneTileLuminance(Src, 32);

			// Copy the decompressed tile back to the output buffer
			srcPtr = Src;
			destPtr = &outBuf[(tileY * 8 * width + tileX * 8) * 4];
			for (int row = 0; row < tileHeight; ++row) {
				memcpy(destPtr, srcPtr, tileWidth * 4);
				srcPtr += 32; // 8 * 4
				destPtr += width * 4;
			}
		}
	}
}

/*
===================
idBareDctDecoder::DecompressOneTileLuminance
===================
*/
void idBareDctDecoder::DecompressOneTileLuminance( uint8_t *YCoCg, int stride ) {
	int16_t coef[64];
	HuffmanDecode(coef, &huffTableYDC, &huffTableYAC, &dcY);
	IDCT_AP922_float(reinterpret_cast<const char*>(coef), reinterpret_cast<const char*>(quantTableY), coef);

	uint8_t *limitTable = &rangeLimitTable[256];
	int16_t *coefPtr = coef;

	for (int row = 0; row < 8; ++row) {
		YCoCg[0] = limitTable[coefPtr[0] + 128 + YCoCg[0]];
		YCoCg[4] = limitTable[coefPtr[1] + 128 + YCoCg[4]];
		YCoCg[8] = limitTable[coefPtr[2] + 128 + YCoCg[8]];
		YCoCg[12] = limitTable[coefPtr[3] + 128 + YCoCg[12]];
		YCoCg[16] = limitTable[coefPtr[4] + 128 + YCoCg[16]];
		YCoCg[20] = limitTable[coefPtr[5] + 128 + YCoCg[20]];
		YCoCg[24] = limitTable[coefPtr[6] + 128 + YCoCg[24]];
		YCoCg[28] = limitTable[coefPtr[7] + 128 + YCoCg[28]];

		YCoCg += stride;
		coefPtr += 8;
	}
}

/*
===================
idBareDctDecoder::DecompressOneTileRGB
===================
*/
void idBareDctDecoder::DecompressOneTileRGB( uint8_t *rgb, int stride ) {
	int16_t coef[64], coeff[64], dest[64], v9[64], v10[64], v11[320];

	// Decode the Huffman coefficients
	HuffmanDecode(coef, &huffTableYDC, &huffTableYAC, &dcY);
	HuffmanDecode(coeff, &huffTableYDC, &huffTableYAC, &dcY);
	HuffmanDecode(dest, &huffTableYDC, &huffTableYAC, &dcY);
	HuffmanDecode(v9, &huffTableYDC, &huffTableYAC, &dcY);
	HuffmanDecode(v10, &huffTableCoCgDC, &huffTableCoCgAC, &dcCo);
	HuffmanDecode(v11, &huffTableCoCgDC, &huffTableCoCgAC, &dcCg);

	// Perform the Inverse Discrete Cosine Transform
	IDCT_AP922_float(reinterpret_cast<char*>(coef), reinterpret_cast<char*>(quantTableY), coef);
	IDCT_AP922_float(reinterpret_cast<char*>(coeff), reinterpret_cast<char*>(quantTableY), coeff);
	IDCT_AP922_float(reinterpret_cast<char*>(dest), reinterpret_cast<char*>(quantTableY), dest);
	IDCT_AP922_float(reinterpret_cast<char*>(v9), reinterpret_cast<char*>(quantTableY), v9);
	IDCT_AP922_float(reinterpret_cast<char*>(v10), reinterpret_cast<char*>(quantTableCoCg), v10);
	IDCT_AP922_float(reinterpret_cast<char*>(v11), reinterpret_cast<char*>(quantTableCoCg), v11);

	// Convert YCoCg to RGB
	YCoCgToRGB(coef, rgb, stride);
}

/*
===================
idBareDctDecoder::DecompressOneTileRGBA
===================
*/
void idBareDctDecoder::DecompressOneTileRGBA( uint8_t *rgba, int stride ) {
	int16_t coef[64], coeff[64], dest[64], v9[64], v10[64], v11[64], v12[64], v13[64], v14[64], v15[64];

	// Decode the Huffman coefficients
	HuffmanDecode(coef, &huffTableYDC, &huffTableYAC, &dcY);
	HuffmanDecode(coeff, &huffTableYDC, &huffTableYAC, &dcY);
	HuffmanDecode(dest, &huffTableYDC, &huffTableYAC, &dcY);
	HuffmanDecode(v9, &huffTableYDC, &huffTableYAC, &dcY);
	HuffmanDecode(v10, &huffTableCoCgDC, &huffTableCoCgAC, &dcCo);
	HuffmanDecode(v11, &huffTableCoCgDC, &huffTableCoCgAC, &dcCg);

	// Perform the Inverse Discrete Cosine Transform
	IDCT_AP922_float(reinterpret_cast<const char*>(coef), reinterpret_cast<const char*>(quantTableY), coef);
	IDCT_AP922_float(reinterpret_cast<const char*>(coeff), reinterpret_cast<const char*>(quantTableY), coeff);
	IDCT_AP922_float(reinterpret_cast<const char*>(dest), reinterpret_cast<const char*>(quantTableY), dest);
	IDCT_AP922_float(reinterpret_cast<const char*>(v9), reinterpret_cast<const char*>(quantTableY), v9);
	IDCT_AP922_float(reinterpret_cast<const char*>(v10), reinterpret_cast<const char*>(quantTableCoCg), v10);
	IDCT_AP922_float(reinterpret_cast<const char*>(v11), reinterpret_cast<const char*>(quantTableCoCg), v11);

	// Decode the Huffman coefficients for the alpha channel
	HuffmanDecode(v12, &huffTableADC, &huffTableAAC, &dcA);
	HuffmanDecode(v13, &huffTableADC, &huffTableAAC, &dcA);
	HuffmanDecode(v14, &huffTableADC, &huffTableAAC, &dcA);
	HuffmanDecode(v15, &huffTableADC, &huffTableAAC, &dcA);

	// Perform the Inverse Discrete Cosine Transform for the alpha channel
	IDCT_AP922_float(reinterpret_cast<const char*>(v12), reinterpret_cast<const char*>(quantTableA), v12);
	IDCT_AP922_float(reinterpret_cast<const char*>(v13), reinterpret_cast<const char*>(quantTableA), v13);
	IDCT_AP922_float(reinterpret_cast<const char*>(v14), reinterpret_cast<const char*>(quantTableA), v14);
	IDCT_AP922_float(reinterpret_cast<const char*>(v15), reinterpret_cast<const char*>(quantTableA), v15);

	// Convert YCoCgA to RGBA
	YCoCgAToRGBA(coef, rgba, stride);
}

/*
===================
idBareDctDecoder::DecompressOneTileYCoCg
===================
*/
void idBareDctDecoder::DecompressOneTileYCoCg( uint8_t *rgb, int stride ) {
	int16_t coef[64], coeff[64], dest[64], v9[64], v10[64], v11[320];

	// Decode the Huffman coefficients
	HuffmanDecode(coef, &huffTableYDC, &huffTableYAC, &dcY);
	HuffmanDecode(coeff, &huffTableYDC, &huffTableYAC, &dcY);
	HuffmanDecode(dest, &huffTableYDC, &huffTableYAC, &dcY);
	HuffmanDecode(v9, &huffTableYDC, &huffTableYAC, &dcY);
	HuffmanDecode(v10, &huffTableCoCgDC, &huffTableCoCgAC, &dcCo);
	HuffmanDecode(v11, &huffTableCoCgDC, &huffTableCoCgAC, &dcCg);

	// Perform the Inverse Discrete Cosine Transform
	IDCT_AP922_float(reinterpret_cast<const char*>(coef), reinterpret_cast<const char*>(quantTableY), coef);
	IDCT_AP922_float(reinterpret_cast<const char*>(coeff), reinterpret_cast<const char*>(quantTableY), coeff);
	IDCT_AP922_float(reinterpret_cast<const char*>(dest), reinterpret_cast<const char*>(quantTableY), dest);
	IDCT_AP922_float(reinterpret_cast<const char*>(v9), reinterpret_cast<const char*>(quantTableY), v9);
	IDCT_AP922_float(reinterpret_cast<const char*>(v10), reinterpret_cast<const char*>(quantTableCoCg), v10);
	IDCT_AP922_float(reinterpret_cast<const char*>(v11), reinterpret_cast<const char*>(quantTableCoCg), v11);

	// Store YCoCg data to RGB
	StoreYCoCg(coef, rgb, stride);
}

/*
===================
idBareDctDecoder::SetRangeTable
===================
*/
void idBareDctDecoder::SetRangeTable( void ) {
	uint8_t *v2;
	int i;

	memset(this->rangeLimitTable, 0, 0x100);
	v2 = &this->rangeLimitTable[256];
	for ( i = 0; i < 256; ++i ) {
		v2[i] = i;
	}
	memset(&this->rangeLimitTable[512], 255, 0x180);
	memset(&this->rangeLimitTable[896], 0, 0x180);
	memcpy(&this->rangeLimitTable[1280], v2, 0x80);
}

/*
===================
idBareDctDecoder::StoreYCoCg
===================
*/
void idBareDctDecoder::StoreYCoCg( const int16_t *YCoCg, uint8_t *rgb, int stride ) {
	const int16_t *yPtr = YCoCg;
	int blockCount = 0;

	do {
		int yOffset = (blockCount & 2) ? 16 : 0;
		int xOffset = (blockCount & 1) ? 16 : 0;
		uint8_t *rgbPtr = &rgb[xOffset * 4 + yOffset * stride];

		for (int k = 0; k < 4; ++k) {
			for (int j = 0; j < 4; ++j) {
				for (int i = 0; i < 4; ++i) {
					int index = (k * 4 + j * 8 + i);
					int yc = yPtr[index];
					int co = rangeLimitTable[yPtr[64 + index] + 384];
					int cg = rangeLimitTable[yPtr[128 + index] + 384];

					rgbPtr[0] = rangeLimitTable[yc + 384];
					rgbPtr[1] = co;
					rgbPtr[2] = cg;
					rgbPtr[3] = 255;

					rgbPtr += 4;
				}
				rgbPtr += stride - 16;
			}
		}

		yPtr += 64;
		++blockCount;
	} while (blockCount < 4);
}

/*
===================
idBareDctDecoder::YCoCgAToRGBA
===================
*/
void idBareDctDecoder::YCoCgAToRGBA( const int16_t *YCoCgA, uint8_t *rgba, int stride ) {
	int blockCount = 0;
	const int16_t *yPtr = YCoCgA + 1;

	do {
		int yOffset = (blockCount & 2) ? 16 : 0;
		int xOffset = (blockCount & 1) ? 16 : 0;
		uint8_t *rgbaPtr = &rgba[xOffset * 4 + yOffset * stride];

		for (int k = 0; k < 4; ++k) {
			for (int j = 0; j < 4; ++j) {
				for (int i = 0; i < 4; ++i) {
					int index = (k * 4 + j * 8 + i);
					int y = yPtr[index];
					int co = yPtr[64 + index];
					int cg = yPtr[128 + index];
					int alpha = yPtr[192 + index];

					int r = y - co + cg;
					int g = y + cg;
					int b = y + co + cg;
					int a = alpha;

					rgbaPtr[0] = rangeLimitTable[r + 384];
					rgbaPtr[1] = rangeLimitTable[g + 384];
					rgbaPtr[2] = rangeLimitTable[b + 384];
					rgbaPtr[3] = rangeLimitTable[a + 384];

					rgbaPtr += 4;
				}
				rgbaPtr += stride - 16;
			}
		}

		yPtr += 64;
		++blockCount;
	} while (blockCount < 4);
}

/*
===================
idBareDctDecoder::YCoCgToRGB
===================
*/
void idBareDctDecoder::YCoCgToRGB( const int16_t *YCoCg, uint8_t *rgb, int stride ) {
	int blockCount = 0;
	const int16_t *yPtr = YCoCg + 8;

	do {
		int yOffset = (blockCount & 2) ? 16 : 0;
		int xOffset = (blockCount & 1) ? 16 : 0;
		uint8_t *rgbPtr = &rgb[xOffset * 4 + yOffset * stride];
		const int16_t *yBlock = &YCoCg[16 * yOffset + 256 + 4 * xOffset];
		const int16_t *cPtr = yPtr;

		for (int k = 0; k < 4; ++k) {
			for (int j = 0; j < 4; ++j) {
				for (int i = 0; i < 4; ++i) {
					int index = (k * 4 + j * 8 + i);
					int yc = yBlock[index];
					int co = cPtr[index - 8];
					int cg = cPtr[index];
					int r = yc - co + cg;
					int g = yc + cg;
					int b = yc + co + cg;

					rgbPtr[0] = rangeLimitTable[r + 384];
					rgbPtr[1] = rangeLimitTable[g + 384];
					rgbPtr[2] = rangeLimitTable[b + 384];
					rgbPtr[3] = 255;

					rgbPtr += 4;
				}
				rgbPtr += stride - 16;
			}
			yBlock += 8;
			cPtr += 16;
		}

		yPtr += 64;
		++blockCount;
	} while (blockCount < 4);
}