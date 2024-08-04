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
#include "TabWindow.h"
#include "TabContainerWindow.h"

void hhTabWindow::CommonInit()
{
	active = false;
}

hhTabWindow::hhTabWindow(idDeviceContext *d, idUserInterfaceLocal *g) : idWindow(d, g)
{
	dc = d;
	gui = g;
	CommonInit();
}

hhTabWindow::hhTabWindow(idUserInterfaceLocal *g) : idWindow(g)
{
	gui = g;
	CommonInit();
}

const char *hhTabWindow::HandleEvent(const sysEvent_t *event, bool *updateVisuals)
{
	// need to call this to allow proper focus and capturing on embedded children
	const char *ret = idWindow::HandleEvent(event, updateVisuals);

	return ret;
}


bool hhTabWindow::ParseInternalVar(const char *_name, idParser *src)
{
	return idWindow::ParseInternalVar(_name, src);
}

void hhTabWindow::PostParse()
{
	idWindow::PostParse();
}

void hhTabWindow::Draw(int time, float x, float y)
{
	if(!active)
		return;
	//idWindow::Draw(time, x, y);
}

void hhTabWindow::Activate(bool activate, idStr &act)
{
	idWindow::Activate(activate, act);

	if (activate) {
		UpdateTab();
	}
}

void hhTabWindow::UpdateTab()
{
	visible = active;
	SetVisible(active);
}

void hhTabWindow::StateChanged(bool redraw)
{
	UpdateTab();
}

void hhTabWindow::SetActive(bool b)
{
	active = b;
	if(active)
		RunScript(ON_TABACTIVATE);
	SetVisible(b);
}

void hhTabWindow::SetOffsets(float x, float y)
{
	for(int i = 0; i < children.Num(); i++)
	{
		hhTabContainerWindow *tabContainer = dynamic_cast<hhTabContainerWindow *>(children[i]);
		if(tabContainer)
			tabContainer->SetOffsets(x, y);
	}
}

void hhTabWindow::SetVisible(bool on)
{
	idWindow::SetVisible(active && on);
}