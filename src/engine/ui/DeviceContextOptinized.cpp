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

#include "DeviceContext.h"
#include "../renderer/GuiModel.h"

// bypass rendersystem to directly work on guiModel
extern idGuiModel * tr_guiModel;

/*
================================================================================================

OPTIMIZED VERSIONS

================================================================================================
*/

// this is only called for the cursor and debug strings, and it should
// scope properly with push/pop clipRect
void idDeviceContextOptimized::EnableClipping(bool b) {
	if ( b == enableClipping ) {
		return;
	}
	enableClipping = b;
	if ( !enableClipping ) {
		PopClipRect();
	} else {
		// the actual value of the rect is irrelvent
		PushClipRect( idRectangle( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT ) );

		// allow drawing beyond the normal bounds for debug text
		// this also allows the cursor to draw outside, so we might want
		// to make this exactly the screen bounds, since we aren't likely
		// to ever turn on the gui debug text again...
		clipX1 = -SCREEN_WIDTH;
		clipX2 = SCREEN_WIDTH * 2;
		clipY1 = -SCREEN_HEIGHT;
		clipY2 = SCREEN_HEIGHT * 2;
	}
};


void idDeviceContextOptimized::PopClipRect() {
	if (clipRects.Num()) {
		clipRects.SetNum( clipRects.Num()-1 );	// don't resize the list, just change num
	}
	if ( clipRects.Num() > 0 ) {
		const idRectangle & clipRect = clipRects[ clipRects.Num() - 1 ];
		clipX1 = clipRect.x;
		clipY1 = clipRect.y;
		clipX2 = clipRect.x + clipRect.w;
		clipY2 = clipRect.y + clipRect.h;
	} else {
		clipX1 = 0;
		clipY1 = 0;
		clipX2 = SCREEN_WIDTH;
		clipY2 = SCREEN_HEIGHT;
	}
}

static const idRectangle baseScreenRect( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT );

void idDeviceContextOptimized::PushClipRect(idRectangle r) {
	const idRectangle & prev = ( clipRects.Num() == 0 ) ? baseScreenRect : clipRects[clipRects.Num()-1];

	// instead of storing the rect, store the intersection of the rect
	// with the previous rect, so ClippedCoords() only has to test against one rect
	idRectangle intersection = prev;
	intersection.ClipAgainst( r, false );
	clipRects.Append( intersection );

	const idRectangle & clipRect = clipRects[ clipRects.Num() - 1 ];
	clipX1 = clipRect.x;
	clipY1 = clipRect.y;
	clipX2 = clipRect.x + clipRect.w;
	clipY2 = clipRect.y + clipRect.h;
}

bool idDeviceContextOptimized::ClippedCoords(float *x, float *y, float *w, float *h, float *s1, float *t1, float *s2, float *t2) {
	const float ox = *x;
	const float oy = *y;
	const float ow = *w;
	const float oh = *h;

	// presume visible first
	if ( ox >= clipX1 && oy >= clipY1 && ox + ow <= clipX2 && oy + oh <= clipY2 ) {
		return false;
	}

	// do clipping
	if ( ox < clipX1 ) {
		*w -= clipX1 - ox;
		*x = clipX1;
	} else if ( ox > clipX2 ) {
		return true;
	}
	if ( oy < clipY1) {
		*h -= clipY1 - oy;
		*y = clipY1;
	} else if ( oy > clipY2) {
		return true;
	}
	if ( *x + *w > clipX2 ) {
		*w = clipX2 - *x;
	}
	if ( *y + *h > clipY2 ) {
		*h = clipY2 - *y;
	}

	if ( *w <= 0 || *h <= 0 ) {
		return true;
	}

	// filled rects won't pass in texcoords
	if ( s1 ) {
		float ns1, ns2, nt1, nt2;
		// upper left
		float u = ( *x - ox ) / ow;
		ns1 = *s1 * ( 1.0f - u ) + *s2 * ( u );

		// upper right
		u = ( *x + *w - ox ) / ow;
		ns2 = *s1 * ( 1.0f - u ) + *s2 * ( u );

		// lower left
		u = ( *y - oy ) / oh;
		nt1 = *t1 * ( 1.0f - u ) + *t2 * ( u );

		// lower right
		u = ( *y + *h - oy ) / oh;
		nt2 = *t1 * ( 1.0f - u ) + *t2 * ( u );

		// set values
		*s1 = ns1;
		*s2 = ns2;
		*t1 = nt1;
		*t2 = nt2;
	}

	// still needs to be drawn
	return false;
}

/*
=============
idDeviceContextOptimized::DrawText
=============
*/
static triIndex_t quadPicIndexes[6] = { 3, 0, 2, 2, 0, 1 };
int idDeviceContextOptimized::DrawText(float x, float y, float scale, idVec4 color, const char *text, float adjust, int limit, int style, int cursor) {
	if ( !matIsIdentity || cursor != -1 ) {
		// fallback to old code
		return idDeviceContextLocal::DrawText( x, y, scale, color, text, adjust, limit, style, cursor );
	}

	idStr drawText = text;

	if ( drawText.Length() == 0 ) {
		return 0;
	}
	if ( color.w == 0.0f ) {
		return 0;
	}

	const uint32 currentColor = PackColor( color );
	uint32 currentColorNativeByteOrder = LittleLong( currentColor );

	int len = drawText.Length();
	if (limit > 0 && len > limit) {
		len = limit;
	}

	int charIndex = 0;
	while ( charIndex < drawText.Length() ) {
		uint32 textChar = drawText.UTF8Char( charIndex );
		if ( textChar == C_COLOR_ESCAPE ) {
			// I'm not sure if inline text color codes are used anywhere in the game,
			// they may only be needed for multi-color user names
			idVec4		newColor;
			uint32 colorIndex = drawText.UTF8Char( charIndex );
			if ( colorIndex == C_COLOR_DEFAULT ) {
				newColor = color;
			} else {
				newColor = idStr::ColorForIndex( colorIndex );
				newColor[3] = color[3];
			}
			renderSystem->SetColor(newColor);
			currentColorNativeByteOrder = LittleLong( PackColor( newColor ) );
			continue;
		}

		scaledGlyphInfo_t glyphInfo;
		activeFont->GetScaledGlyph( scale, textChar, glyphInfo );

		// PaintChar( x, y, glyphInfo );
		float drawY = y - glyphInfo.top;
		float drawX = x + glyphInfo.left;
		float w = glyphInfo.width;
		float h = glyphInfo.height;
		float s = glyphInfo.s1;
		float t = glyphInfo.t1;
		float s2 = glyphInfo.s2;
		float t2 = glyphInfo.t2;

		if ( !ClippedCoords( &drawX, &drawY, &w, &h, &s, &t, &s2, &t2) ) {
			float x1 = xOffset + drawX * xScale;
			float x2 = xOffset + ( drawX + w ) * xScale;
			float y1 = yOffset + drawY * yScale;
			float y2 = yOffset + ( drawY + h ) * yScale;
			idDrawVert * verts = tr_guiModel->AllocTris( 4, quadPicIndexes, 6, glyphInfo.material, 0 );
			if ( verts != NULL ) {
				verts[0].xyz[0] = x1;
				verts[0].xyz[1] = y1;
				verts[0].xyz[2] = 0.0f;
				verts[0].SetTexCoord( s, t );
				verts[0].SetNativeOrderColor( currentColorNativeByteOrder );
				verts[0].ClearColor2();

				verts[1].xyz[0] = x2;
				verts[1].xyz[1] = y1;
				verts[1].xyz[2] = 0.0f;
				verts[1].SetTexCoord( s2, t );
				verts[1].SetNativeOrderColor( currentColorNativeByteOrder );
				verts[1].ClearColor2();

				verts[2].xyz[0] = x2;
				verts[2].xyz[1] = y2;
				verts[2].xyz[2] = 0.0f;
				verts[2].SetTexCoord( s2, t2 );
				verts[2].SetNativeOrderColor( currentColorNativeByteOrder );
				verts[2].ClearColor2();

				verts[3].xyz[0] = x1;
				verts[3].xyz[1] = y2;
				verts[3].xyz[2] = 0.0f;
				verts[3].SetTexCoord( s, t2 );
				verts[3].SetNativeOrderColor( currentColorNativeByteOrder );
				verts[3].ClearColor2();
			}
		}

		x += glyphInfo.xSkip + adjust;
	}
	return drawText.Length();
}
