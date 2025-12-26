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

// for gui tabContainerDef
#ifndef __TABCONTAINERWINDOW_H__
#define __TABCONTAINERWINDOW_H__

class hhTabWindow;

/*
 * tabContainerDef
 *  Widget with many tabs(tabDef)
 */
class hhTabContainerWindow : public idWindow {
	public:
		hhTabContainerWindow(idUserInterfaceLocal *gui);
		hhTabContainerWindow(idDeviceContext *d, idUserInterfaceLocal *gui);

		virtual const char	*HandleEvent(const sysEvent_t *event, bool *updateVisuals);
		virtual void		PostParse(void);
		virtual void		Draw(int time, float x, float y);
		virtual void		Activate(bool activate, idStr &act);
		virtual void		StateChanged(bool redraw = false);

		void				UpdateTab(bool onlyOffset = false);
        void 				SetOffsets(float x, float y);
        virtual idWinVar *  GetWinVarByName(const char *_name, bool winLookup = false, drawWin_t **owner = NULL);

	private:
		virtual bool		ParseInternalVar(const char *name, idParser *src);
		void				CommonInit(void);
        void 				SetActiveTab(int index);
		float				GetTabHeight(void);
        float				GetTabWidth(void);
        bool 				ButtonContains(const hhTabWindow *tab);

        idWinInt            activeTab; // RW; non-ref; script;
        idVec2              tabMargins; // RO; non-ref; non-script;
        idVec4              sepColor; // RO; non-ref; non-script;
	    // bool				horizontal;
        bool    			vertical; // RO; non-ref; non-script; can't auto parsing
        float			    tabHeight; // RO; non-ref; non-script;

		idList<hhTabWindow *> tabs;
        int 				currentTab;
        idVec2 				offsets;
};

#endif // __TABCONTAINERWINDOW_H__
