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

#include "Window.h"
#include "UserInterfaceLocal.h"
#include "ButtonWindow.h"

void hhButtonWindow::CommonInit(void)
{
    idWindow::CommonInit();

    buttonMat.Reset();
    edgeWidth = -1.0f;
    hoverBorderColor = idVec4(1.0f, 1.0f, 1.0f, 1.0f);
}

hhButtonWindow::hhButtonWindow(idDeviceContext *d, idUserInterfaceLocal *g) : idWindow(d, g)
{
	dc = d;
	gui = g;
	CommonInit();
}

hhButtonWindow::hhButtonWindow(idUserInterfaceLocal *g) : idWindow(g)
{
	gui = g;
	CommonInit();
}

bool hhButtonWindow::ParseInternalVar(const char *_name, idParser *src)
{
    if (idStr::Icmp(_name, "edgeWidth") == 0)
{
        edgeWidth = src->ParseFloat();
        return true;
}
    if (idStr::Icmp(_name, "hoverBorderColor") == 0)
{
        hoverBorderColor[0] = src->ParseFloat();
        src->ExpectTokenString(",");
        hoverBorderColor[1] = src->ParseFloat();
        src->ExpectTokenString(",");
        hoverBorderColor[2] = src->ParseFloat();
        src->ExpectTokenString(",");
        hoverBorderColor[3] = src->ParseFloat();
        return true;
    }
    return idWindow::ParseInternalVar(_name, src);
}

idWinVar * hhButtonWindow::GetWinVarByName(const char *_name, bool fixup, drawWin_t **owner)
{
    if (idStr::Icmp(_name, "leftMat") == 0)
    {
        return &buttonMat.left.name;
    }
    if (idStr::Icmp(_name, "middleMat") == 0)
    {
        return &buttonMat.middle.name;
    }
    if (idStr::Icmp(_name, "rightMat") == 0)
    {
        return &buttonMat.right.name;
    }

    return idWindow::GetWinVarByName(_name, fixup, owner);
}

void hhButtonWindow::PostParse(void)
{
    idWindow::PostParse();

    buttonMat.Setup(edgeWidth);
}

void hhButtonWindow::Draw(int time, float x, float y)
{
    idRectangle r = rect;
    r.Offset(x, y);
    buttonMat.Draw(dc, r, false, matScalex, matScaley, flags, matColor);
	idWindow::Draw(time, x, y);
}
