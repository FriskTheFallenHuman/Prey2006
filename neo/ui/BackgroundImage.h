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

// for gui left-middle-right background widget
#ifndef __BACKGROUNDIMAGE_H__
#define __BACKGROUNDIMAGE_H__

#include "../renderer/DeviceContext.h"
#include "Winvar.h"

/*
* Background image struct
*  Like idWindow::backGroundName and idWindow::background
*/
class hhBackground {
public:
	void Reset( void );
    void Draw(idDeviceContext *dc, const idRectangle &drawRect, float matScalex, float matScaley, unsigned int flags, const idVec4 &color = idVec4(1.0f, 1.0f, 1.0f, 1.0f));
	void Setup( void );

	idWinBackground           name;
	const idMaterial          *material;
};



/*
* Complex background image
*  Horizontal: left middle right
*  Vertical: top middle bottom
*  If left image is unset, using right mirror image
*  If right image is unset, using left mirror image
*/
class hhBackgroundGroup {
public:
	void Reset( void );
    void Draw(idDeviceContext *dc, const idRectangle &total, bool vertical, float matScalex, float matScaley, unsigned int flags, const idVec4 &color = idVec4(1.0f, 1.0f, 1.0f, 1.0f));
	void Setup( float edge = -1.0f );

	static int CalcRects(const idRectangle &total, hhBackground *bgs[3], idRectangle rects[3], bool vertical, float edgeWidth = -1.0f);

	hhBackground           left, middle, right;
	float                  edge;
private:
	enum {
		BG_NONE = 0,
		BG_LEFT = BIT(0),
		BG_MIDDLE = BIT(1),
		BG_RIGHT = BIT(2),
		BG_LEFT_MIRROR = BIT(3), // when right image unset
		BG_RIGHT_MIRROR = BIT(4), // when left image unset
	};
};

#endif // __BACKGROUNDIMAGE_H__
