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

#ifndef __BAREDCTDECODER__
#define __BAREDCTDECODER__

void IDCT_Init( void );
void IDCT_AP922_float( const char *coeff, const char *quant, int16_t *dest );

class idBareDCTHuffmanTable {
public:
    uint8_t bits[17];
    uint8_t symbols[256];
    unsigned int code[256];
    char size[256];
    int minCode[17];
    int maxCode[18];
    int symOffset[17];
    uint8_t look_nbits[256];
    uint8_t look_sym[256];
    int test_nbits[16];

    void Init( const uint8_t *bits, const uint8_t *symbols );
};

class idBareDctBase {
public:
    int luminanceQuality;
    int chrominanceQuality;
    int alphaQuality;
    uint16_t *quantTableY;
    uint16_t *quantTableCoCg;
    uint16_t *quantTableA;

    idBareDCTHuffmanTable huffTableYDC;
    idBareDCTHuffmanTable huffTableYAC;
    idBareDCTHuffmanTable huffTableCoCgDC;
    idBareDCTHuffmanTable huffTableCoCgAC;
    idBareDCTHuffmanTable huffTableADC;
    idBareDCTHuffmanTable huffTableAAC;

    void InitHuffmanTable( void );
    void InitQuantTable( void );
    int QuantizationScaleFromQuality( int quality );
    void ScaleQuantTable( uint16_t *quantTable, const uint16_t *srcTable, int scale );
    //void ScaleQuantTable_MMX( uint16_t *quantTable, const uint16_t *srcTable, int scale );
    //void ScaleQuantTable_SSE2( uint16_t *quantTable, const uint16_t *srcTable, int scale );
    void SetQuality_Generic( int luminance, int chrominance, int alpha );
    //void SetQuality_MMX( int luminance, int chrominance, int alpha );
    //void SetQuality_SSE2( int luminance, int chrominance, int alpha );
    //void SetQuality_Xenon( int luminance, int chrominance, int alpha );
};

class idBareDctDecoder : public idBareDctBase {
public:
    int imageWidth;
    int imageHeight;
    int dcY;
    int dcCo;
    int dcCg;
    int dcA;
    int getBits;
    int getBuff;
    int dataBytes;
    const uint8_t *data;
    uint8_t rangeLimitTable[1408];

    void HuffmanDecode( int16_t *coef, const idBareDCTHuffmanTable *dctbl, const idBareDCTHuffmanTable *actbl, int *lastDC );

    int DecodeLong_MMX( const idBareDCTHuffmanTable *htbl );
    void DecompressImageRGBA_Generic( const uint8_t *data, uint8_t *dest, int width, int height, int stride );
    void DecompressImageRGB_Generic( const uint8_t *data, uint8_t *dest, int width, int height, int stride );
    void DecompressImageYCoCg_Generic(const uint8_t *data, uint8_t *dest, int width, int height, int stride );
    //void DecompressImageYCoCg_Xenon( const uint8_t *data, uint8_t *dest, int width, int height, int stride );
    void DecompressLuminanceEnhancement_Generic( const uint8_t *data, uint8_t* dest, int width, int height, int stride );
    void DecompressOneTileLuminance( uint8_t *dest, int stride );
    void DecompressOneTileRGB( uint8_t *dest, int stride );
    void DecompressOneTileRGBA( uint8_t *dest, int stride );
    void DecompressOneTileYCoCg(uint8_t *dest, int stride );
    void SetRangeTable( void );
    void StoreYCoCg( const int16_t *block, uint8_t *dest, int stride );
    void YCoCgAToRGBA( const int16_t *block, uint8_t *dest, int stride );
    void YCoCgToRGB( const int16_t *block, uint8_t *dest, int stride );
};

#endif /* !__BAREDCTDECODER__ */