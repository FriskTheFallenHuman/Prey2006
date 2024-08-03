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

float tg_1_16_0 = tan(0.1963495463132858);
float tg_3_16_0 = tan(0.5890486389398575);
float tg_2_16_0 = tan(0.3926990926265717);
float cos_4_16_0 = cos(0.7853981852531433);

const int TABLE_SIZE = 32;
float tab_i_04_0[TABLE_SIZE];
float tab_i_17_0[TABLE_SIZE];
float tab_i_26_0[TABLE_SIZE];
float tab_i_35_0[TABLE_SIZE];

float* inverseRowTables[8] = {
	&tab_i_04_0[0], &tab_i_17_0[0], &tab_i_26_0[0],
	&tab_i_35_0[0],&tab_i_04_0[0],&tab_i_35_0[0],
	&tab_i_26_0[0],&tab_i_17_0[0]
};

unsigned short BICUBIC_INT_0 = 0x51;

/*
========================
IDCT_Init
========================
*/
void IDCT_Init( void ) {
	long double cos_values[8];
	cos_values[0] = cos(0.1963495463132858); // cos(π/16)
	cos_values[1] = cos(0.3926990926265717); // cos(π/8)
	cos_values[2] = cos(0.5890486389398575); // cos(3π/16)
	cos_values[3] = cos(0.7853981852531433); // cos(π/4)
	cos_values[4] = cos(0.9817477315664291); // cos(5π/16)
	cos_values[5] = cos(1.178097277879715);  // cos(3π/8)
	cos_values[6] = cos(1.374446824193001);  // cos(7π/16)
	cos_values[7] = cos(1.570796370506287);  // cos(π/2)

	long double factor_04 = cos_values[3] * cos_values[3] * 0.25;
	tab_i_04_0[0] = factor_04;
	tab_i_04_0[1] = factor_04;
	tab_i_04_0[2] = factor_04;
	tab_i_04_0[3] = factor_04;
	tab_i_04_0[4] = cos_values[1] * cos_values[3] * 0.25;
	tab_i_04_0[5] = cos_values[5] * cos_values[3] * 0.25;
	tab_i_04_0[6] = -tab_i_04_0[5];
	tab_i_04_0[7] = -tab_i_04_0[4];
	tab_i_04_0[8] = factor_04;
	tab_i_04_0[9] = -factor_04;
	tab_i_04_0[10] = -factor_04;
	tab_i_04_0[11] = factor_04;
	tab_i_04_0[12] = tab_i_04_0[5];
	tab_i_04_0[13] = -tab_i_04_0[4];
	tab_i_04_0[14] = tab_i_04_0[4];
	tab_i_04_0[15] = -tab_i_04_0[5];
	tab_i_04_0[16] = cos_values[0] * cos_values[3] * 0.25;
	tab_i_04_0[17] = cos_values[2] * cos_values[3] * 0.25;
	tab_i_04_0[18] = cos_values[4] * cos_values[3] * 0.25;
	tab_i_04_0[19] = cos_values[6] * cos_values[3] * 0.25;
	tab_i_04_0[20] = tab_i_04_0[17];
	tab_i_04_0[21] = -tab_i_04_0[19];
	tab_i_04_0[22] = -tab_i_04_0[16];
	tab_i_04_0[23] = -tab_i_04_0[18];
	tab_i_04_0[24] = tab_i_04_0[18];
	tab_i_04_0[25] = -tab_i_04_0[16];
	tab_i_04_0[26] = tab_i_04_0[19];
	tab_i_04_0[27] = tab_i_04_0[17];
	tab_i_04_0[28] = tab_i_04_0[19];
	tab_i_04_0[29] = -tab_i_04_0[18];
	tab_i_04_0[30] = tab_i_04_0[17];
	tab_i_04_0[31] = -tab_i_04_0[16];

	long double factor_17 = cos_values[3] * cos_values[0] * 0.25;
	tab_i_17_0[0] = factor_17;
	tab_i_17_0[1] = factor_17;
	tab_i_17_0[2] = factor_17;
	tab_i_17_0[3] = factor_17;
	tab_i_17_0[4] = cos_values[1] * cos_values[0] * 0.25;
	tab_i_17_0[5] = cos_values[5] * cos_values[0] * 0.25;
	tab_i_17_0[6] = -tab_i_17_0[5];
	tab_i_17_0[7] = -tab_i_17_0[4];
	tab_i_17_0[8] = factor_17;
	tab_i_17_0[9] = -factor_17;
	tab_i_17_0[10] = -factor_17;
	tab_i_17_0[11] = factor_17;
	tab_i_17_0[12] = tab_i_17_0[5];
	tab_i_17_0[13] = -tab_i_17_0[4];
	tab_i_17_0[14] = tab_i_17_0[4];
	tab_i_17_0[15] = -tab_i_17_0[5];
	tab_i_17_0[16] = cos_values[0] * cos_values[0] * 0.25;
	tab_i_17_0[17] = cos_values[2] * cos_values[0] * 0.25;
	tab_i_17_0[18] = cos_values[4] * cos_values[0] * 0.25;
	tab_i_17_0[19] = cos_values[6] * cos_values[0] * 0.25;
	tab_i_17_0[20] = tab_i_17_0[17];
	tab_i_17_0[21] = -tab_i_17_0[19];
	tab_i_17_0[22] = -tab_i_17_0[16];
	tab_i_17_0[23] = -tab_i_17_0[18];
	tab_i_17_0[24] = tab_i_17_0[18];
	tab_i_17_0[25] = -tab_i_17_0[16];
	tab_i_17_0[26] = tab_i_17_0[19];
	tab_i_17_0[27] = tab_i_17_0[17];
	tab_i_17_0[28] = tab_i_17_0[19];
	tab_i_17_0[29] = -tab_i_17_0[18];
	tab_i_17_0[30] = tab_i_17_0[17];
	tab_i_17_0[31] = -tab_i_17_0[16];

	long double factor_26 = cos_values[3] * cos_values[1] * 0.25;
	tab_i_26_0[0] = factor_26;
	tab_i_26_0[1] = factor_26;
	tab_i_26_0[2] = factor_26;
	tab_i_26_0[3] = factor_26;
	tab_i_26_0[4] = cos_values[1] * cos_values[1] * 0.25;
	tab_i_26_0[5] = cos_values[5] * cos_values[1] * 0.25;
	tab_i_26_0[6] = -tab_i_26_0[5];
	tab_i_26_0[7] = -tab_i_26_0[4];
	tab_i_26_0[8] = factor_26;
	tab_i_26_0[9] = -factor_26;
	tab_i_26_0[10] = -factor_26;
	tab_i_26_0[11] = factor_26;
	tab_i_26_0[12] = tab_i_26_0[5];
	tab_i_26_0[13] = -tab_i_26_0[4];
	tab_i_26_0[14] = tab_i_26_0[4];
	tab_i_26_0[15] = -tab_i_26_0[5];
	tab_i_26_0[16] = cos_values[0] * cos_values[1] * 0.25;
	tab_i_26_0[17] = cos_values[2] * cos_values[1] * 0.25;
	tab_i_26_0[18] = cos_values[4] * cos_values[1] * 0.25;
	tab_i_26_0[19] = cos_values[6] * cos_values[1] * 0.25;
	tab_i_26_0[20] = tab_i_26_0[17];
	tab_i_26_0[21] = -tab_i_26_0[19];
	tab_i_26_0[22] = -tab_i_26_0[16];
	tab_i_26_0[23] = -tab_i_26_0[18];
	tab_i_26_0[24] = tab_i_26_0[18];
	tab_i_26_0[25] = -tab_i_26_0[16];
	tab_i_26_0[26] = tab_i_26_0[19];
	tab_i_26_0[27] = tab_i_26_0[17];
	tab_i_26_0[28] = tab_i_26_0[19];
	tab_i_26_0[29] = -tab_i_26_0[18];
	tab_i_26_0[30] = tab_i_26_0[17];
	tab_i_26_0[31] = -tab_i_26_0[16];

	long double factor_35 = cos_values[3] * cos_values[2] * 0.25;
	tab_i_35_0[0] = factor_35;
	tab_i_35_0[1] = factor_35;
	tab_i_35_0[2] = factor_35;
	tab_i_35_0[3] = factor_35;
	tab_i_35_0[4] = cos_values[1] * cos_values[2] * 0.25;
	tab_i_35_0[5] = cos_values[5] * cos_values[2] * 0.25;
	tab_i_35_0[6] = -tab_i_35_0[5];
	tab_i_35_0[7] = -tab_i_35_0[4];
	tab_i_35_0[8] = factor_35;
	tab_i_35_0[9] = -factor_35;
	tab_i_35_0[10] = -factor_35;
	tab_i_35_0[11] = factor_35;
	tab_i_35_0[12] = tab_i_35_0[5];
	tab_i_35_0[13] = -tab_i_35_0[4];
	tab_i_35_0[14] = tab_i_35_0[4];
	tab_i_35_0[15] = -tab_i_35_0[5];
	tab_i_35_0[16] = cos_values[0] * cos_values[2] * 0.25;
	tab_i_35_0[17] = cos_values[2] * cos_values[2] * 0.25;
	tab_i_35_0[18] = cos_values[4] * cos_values[2] * 0.25;
	tab_i_35_0[19] = cos_values[6] * cos_values[2] * 0.25;
	tab_i_35_0[20] = tab_i_35_0[17];
	tab_i_35_0[21] = -tab_i_35_0[19];
	tab_i_35_0[22] = -tab_i_35_0[16];
	tab_i_35_0[23] = -tab_i_35_0[18];
	tab_i_35_0[24] = tab_i_35_0[18];
	tab_i_35_0[25] = -tab_i_35_0[16];
	tab_i_35_0[26] = tab_i_35_0[19];
	tab_i_35_0[27] = tab_i_35_0[17];
	tab_i_35_0[28] = tab_i_35_0[19];
	tab_i_35_0[29] = -tab_i_35_0[18];
	tab_i_35_0[30] = tab_i_35_0[17];
	tab_i_35_0[31] = -tab_i_35_0[16];
}

/*
========================
IDCT_AP922_float
========================
*/
void IDCT_AP922_float( const char *coeff, const char* quant, int16_t *dest ) {
	float temp[64];
	const uint16_t * qPtr = reinterpret_cast<const uint16_t *>(quant + 6);
	float** invRowTables = inverseRowTables;
	float* tPtr = temp + 3;
	const int16_t * cPtr = reinterpret_cast<const int16_t *>(coeff + 2);

	for (int i = 0; i < 8; ++i) {
		float t6a = *(qPtr - 3) * *(cPtr - 1);
		float t5 = *cPtr * *reinterpret_cast<const uint16_t *>(quant);
		float t0a = *(qPtr - 1) * cPtr[1];
		float tm12 = *qPtr * cPtr[2];
		float t1a = qPtr[1] * cPtr[3];
		float* invRow = *invRowTables;
		float tp465 = qPtr[2] * cPtr[4];
		float t2a = qPtr[3] * cPtr[5];
		float tm03a = qPtr[4] * cPtr[6];

		float t0b = invRow[12] * t2a + invRow[8] * t1a + invRow[4] * t0a + t6a * invRow[0];
		float t6b = invRow[5] * t0a + invRow[1] * t6a + invRow[9] * t1a + invRow[13] * t2a;
		float tp65a = invRow[6] * t0a + invRow[2] * t6a + invRow[10] * t1a + invRow[14] * t2a;
		float t2b = t1a * invRow[11] + t6a * invRow[3] + t0a * invRow[7] + t2a * invRow[15];

		float tp465a = invRow[28] * tm03a + invRow[24] * tp465 + invRow[16] * t5 + invRow[20] * tm12;
		float t5a = invRow[21] * tm12 + invRow[17] * t5 + invRow[25] * tp465 + invRow[29] * tm03a;
		float tm12a = invRow[22] * tm12 + invRow[18] * t5 + invRow[26] * tp465 + invRow[30] * tm03a;
		float t1b = invRow[19] * t5 + invRow[23] * tm12 + invRow[27] * tp465 + invRow[31] * tm03a;

		tPtr[5] = tp465a + t0b;
		tPtr[6] = t5a + t6b;
		tPtr[7] = tm12a + tp65a;
		tPtr[8] = t1b + t2b;
		tPtr[9] = t2b - t1b;
		tPtr[10] = tp65a - tm12a;
		tPtr[11] = t6b - t5a;
		tPtr[12] = t0b - tp465a;

		invRowTables++;
		tPtr += 8;
		cPtr += 8;
		qPtr += 8;
	}

	float tg1 = tg_1_16_0;
	float tg3 = tg_3_16_0;
	int16_t * dPtr = dest + 16;
	float* tempPtr = temp + 57;

	for (int i = 0; i < 8; ++i) {
		float tp465b = *tempPtr * tg1 + tempPtr[-48];
		float tm12b = tempPtr[-48] * tg1 - *tempPtr;
		float tp65b = tempPtr[-16] * tg3 + tempPtr[-32];
		float t5b = tempPtr[-16] - tempPtr[-32] * tg3;

		float tm03 = tp65b + tp465b;
		float tp465c = tp465b - tp65b;
		temp[0] = t5b + tm12b;
		float tp65c = tm12b - t5b;
		float t0 = (tp65c + tp465c) * cos_4_16_0;
		float t2 = (tp465c - tp65c) * cos_4_16_0;

		float tp465d = tempPtr[-24] + tempPtr[-56];
		float tm12c = tempPtr[-56] - tempPtr[-24];
		float tp65d = tempPtr[-8] * tg_2_16_0 + tempPtr[-40];
		float t5c = tg_2_16_0 * tempPtr[-40] - tempPtr[-8];
		float t4 = tp65d + tp465d;
		float tp65 = tp465d - tp65d;
		float t6 = t5c + tm12c;
		float t1 = tm12c - t5c;

		float val1 = t4 + tm03;
		float val2 = t6 + t0;
		float val3 = t1 + t2;
		float val4 = tp65 + temp[0];
		float val5 = tp65 - temp[0];
		float val6 = t1 - t2;
		float val7 = t6 - t0;
		float val8 = t4 - tm03;

		dPtr[-16] = static_cast<int>(val1 <= 0.0f ? val1 - 0.5f : val1 + 0.5f);
		dPtr[-8] = static_cast<int>(val2 <= 0.0f ? val2 - 0.5f : val2 + 0.5f);
		dPtr[0] = static_cast<int>(val3 <= 0.0f ? val3 - 0.5f : val3 + 0.5f);
		dPtr[8] = static_cast<int>(val4 <= 0.0f ? val4 - 0.5f : val4 + 0.5f);
		dPtr[16] = static_cast<int>(val5 <= 0.0f ? val5 - 0.5f : val5 + 0.5f);
		dPtr[24] = static_cast<int>(val6 <= 0.0f ? val6 - 0.5f : val6 + 0.5f);
		dPtr[32] = static_cast<int>(val7 <= 0.0f ? val7 - 0.5f : val7 + 0.5f);
		dPtr[40] = static_cast<int>(val8 <= 0.0f ? val8 - 0.5f : val8 + 0.5f);

		tempPtr++;
		dPtr++;
	}
}