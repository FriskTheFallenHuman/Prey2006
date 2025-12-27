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

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU
General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "precompiled.h"
#pragma hdrstop

#include "qe3.h"
#include "radiant.h"
#include "RadiantEditor.h"

HINSTANCE	 g_DoomInstance = NULL;
bool		 g_editorAlive	= false;

//
// command mapping stuff m_strCommand is the command string m_nKey is the windows
// VK_??? equivelant m_nModifiers are key states as follows bit 0 - shift 1 - alt
// 2 - control 4 - press only
//

SCommandInfo g_Commands[] = {
	{ "Texture_AxialByHeight", 'U', 0, ID_SELECT_AXIALTEXTURE_BYHEIGHT },
	{ "Texture_AxialArbitrary", 'U', RAD_SHIFT, ID_SELECT_AXIALTEXTURE_ARBITRARY },
	{ "Texture_AxialByWidth", 'U', RAD_CONTROL, ID_SELECT_AXIALTEXTURE_BYWIDTH },
	{ "Texture_Decrement", VK_SUBTRACT, RAD_SHIFT, ID_SELECTION_TEXTURE_DEC },
	{ "Texture_Increment", VK_ADD, RAD_SHIFT, ID_SELECTION_TEXTURE_INC },
	{ "Texture_Fit", '5', RAD_SHIFT, ID_SELECTION_TEXTURE_FIT },
	{ "Texture_RotateClock", VK_NEXT, RAD_SHIFT, ID_SELECTION_TEXTURE_ROTATECLOCK },
	{ "Texture_RotateCounter", VK_PRIOR, RAD_SHIFT, ID_SELECTION_TEXTURE_ROTATECOUNTER },
	{ "Texture_ScaleUp", VK_UP, RAD_CONTROL, ID_SELECTION_TEXTURE_SCALEUP },
	{ "Texture_ScaleDown", VK_DOWN, RAD_CONTROL, ID_SELECTION_TEXTURE_SCALEDOWN },
	{ "Texture_ShiftLeft", VK_LEFT, RAD_SHIFT, ID_SELECTION_TEXTURE_SHIFTLEFT },
	{ "Texture_ShiftRight", VK_RIGHT, RAD_SHIFT, ID_SELECTION_TEXTURE_SHIFTRIGHT },
	{ "Texture_ShiftUp", VK_UP, RAD_SHIFT, ID_SELECTION_TEXTURE_SHIFTUP },
	{ "Texture_ShiftDown", VK_DOWN, RAD_SHIFT, ID_SELECTION_TEXTURE_SHIFTDOWN },
	{ "Texture_ScaleLeft", VK_LEFT, RAD_CONTROL, ID_SELECTION_TEXTURE_SCALELEFT },
	{ "Texture_ScaleRight", VK_RIGHT, RAD_CONTROL, ID_SELECTION_TEXTURE_SCALERIGHT },
	{ "Texture_InvertX", 'I', RAD_CONTROL | RAD_SHIFT, ID_CURVE_NEGATIVETEXTUREY },
	{ "Texture_InvertY", 'I', RAD_SHIFT, ID_CURVE_NEGATIVETEXTUREX },
	{ "Texture_ToggleLock", 'T', RAD_SHIFT, ID_TOGGLE_LOCK },
	{ "Texture_ShowAllTextures", 'A', RAD_CONTROL, ID_TEXTURES_SHOWALL },

	{ "Edit_Copy", 'C', RAD_CONTROL, ID_EDIT_COPYBRUSH },
	{ "Edit_Paste", 'V', RAD_CONTROL, ID_EDIT_PASTEBRUSH },
	{ "Edit_Undo", 'Z', RAD_CONTROL, ID_EDIT_UNDO },
	{ "Edit_Redo", 'Y', RAD_CONTROL, ID_EDIT_REDO },

	{ "Camera_Forward", 'W', 0, ID_CAMERA_FORWARD },
	{ "Camera_Back", 'S', 0, ID_CAMERA_BACK },
	{ "Camera_Left", 'A', 0, ID_CAMERA_LEFT },
	{ "Camera_Right", 'D', 0, ID_CAMERA_RIGHT },
	{ "Camera_Up", 'Q', 0, ID_CAMERA_UP },
	{ "Camera_AngleUp", 'A', 0, ID_CAMERA_ANGLEUP },
	{ "Camera_AngleDown", 'Z', 0, ID_CAMERA_ANGLEDOWN },
	{ "Camera_StrafeRight", VK_PERIOD, 0, ID_CAMERA_STRAFERIGHT },
	{ "Camera_StrafeLeft", VK_COMMA, 0, ID_CAMERA_STRAFELEFT },
	{ "Camera_UpFloor", VK_PRIOR, 0, ID_VIEW_UPFLOOR },
	{ "Camera_DownFloor", VK_NEXT, 0, ID_VIEW_DOWNFLOOR },
	{ "Camera_CenterView", VK_END, 0, ID_VIEW_CENTER },

	{ "ShowHideModels", 'M', RAD_CONTROL, ID_SHOW_MODELS },
	{ "NextView", VK_HOME, 0, ID_VIEW_NEXTVIEW },

	{ "Grid_ZoomOut", VK_INSERT, 0, ID_VIEW_ZOOMOUT },
	{ "Grid_ZoomIn", VK_DELETE, 0, ID_VIEW_ZOOMIN },
	{ "Grid_SetPoint5", '4', RAD_SHIFT, ID_GRID_POINT5 },
	{ "Grid_SetPoint25", '3', RAD_SHIFT, ID_GRID_POINT25 },
	{ "Grid_SetPoint125", '2', RAD_SHIFT, ID_GRID_POINT125 },
	//{ "Grid_SetPoint0625",     '1',			RAD_SHIFT,				ID_GRID_POINT0625 },
	{ "Grid_Set1", '1', 0, ID_GRID_1 },
	{ "Grid_Set2", '2', 0, ID_GRID_2 },
	{ "Grid_Set4", '3', 0, ID_GRID_4 },
	{ "Grid_Set8", '4', 0, ID_GRID_8 },
	{ "Grid_Set16", '5', 0, ID_GRID_16 },
	{ "Grid_Set32", '6', 0, ID_GRID_32 },
	{ "Grid_Set64", '7', 0, ID_GRID_64 },
	{ "Grid_Down", VK_OEM_4, 0, ID_GRID_PREV }, /* [{ in us layout */
	{ "Grid_Up", VK_OEM_6, 0, ID_GRID_NEXT },	/* ]} in US layouts */
	{ "Grid_Toggle", '0', 0, ID_GRID_TOGGLE },
	{ "Grid_ToggleSizePaint", 'Q', RAD_PRESS, ID_SELECTION_TOGGLESIZEPAINT },
	{ "Grid_PrecisionCursorMode", VK_F11, 0, ID_PRECISION_CURSOR_CYCLE },
	{ "Grid_NextView", VK_TAB, RAD_CONTROL, ID_VIEW_NEXTVIEW },
	{ "Grid_ToggleCrosshairs", 'X', RAD_SHIFT, ID_VIEW_CROSSHAIR },
	{ "Grid_ZZoomOut", VK_INSERT, RAD_CONTROL, ID_VIEW_ZZOOMOUT },
	{ "Grid_ZZoomIn", VK_DELETE, RAD_CONTROL, ID_VIEW_ZZOOMIN },

	{ "Brush_Make3Sided", '3', RAD_CONTROL, ID_BRUSH_3SIDED },
	{ "Brush_Make4Sided", '4', RAD_CONTROL, ID_BRUSH_4SIDED },
	{ "Brush_Make5Sided", '5', RAD_CONTROL, ID_BRUSH_5SIDED },
	{ "Brush_Make6Sided", '6', RAD_CONTROL, ID_BRUSH_6SIDED },
	{ "Brush_Make7Sided", '7', RAD_CONTROL, ID_BRUSH_7SIDED },
	{ "Brush_Make8Sided", '8', RAD_CONTROL, ID_BRUSH_8SIDED },
	{ "Brush_Make9Sided", '9', RAD_CONTROL, ID_BRUSH_9SIDED },

	{ "Leak_NextSpot", 'K', RAD_CONTROL | RAD_SHIFT, ID_MISC_NEXTLEAKSPOT },
	{ "Leak_PrevSpot", 'L', RAD_CONTROL | RAD_SHIFT, ID_MISC_PREVIOUSLEAKSPOT },

	{ "File_Open", 'O', RAD_CONTROL, ID_FILE_OPEN },
	{ "File_Save", 'S', RAD_CONTROL, ID_FILE_SAVE },

	{ "TAB", VK_TAB, 0, ID_PATCH_TAB },

	{ "Patch_BendMode", 'B', 0, ID_PATCH_BEND },
	{ "Patch_FreezeVertices", 'F', 0, ID_CURVE_FREEZE },
	{ "Patch_UnFreezeVertices", 'F', RAD_CONTROL, ID_CURVE_UNFREEZE },
	{ "Patch_UnFreezeAllVertices", 'F', RAD_CONTROL | RAD_SHIFT, ID_CURVE_UNFREEZEALL },
	{ "Patch_Thicken", 'T', RAD_CONTROL, ID_CURVE_THICKEN },
	{ "Patch_ClearOverlays", 'Y', RAD_SHIFT, ID_CURVE_OVERLAY_CLEAR },
	{ "Patch_MakeOverlay", 'Y', 0, ID_CURVE_OVERLAY_SET },
	{ "Patch_CycleCapTexturing", 'P', RAD_CONTROL | RAD_SHIFT, ID_CURVE_CYCLECAP },
	{ "Patch_CycleCapTexturingAlt", 'P', RAD_SHIFT, ID_CURVE_CYCLECAPALT },
	{ "Patch_InvertCurve", 'I', RAD_CONTROL, ID_CURVE_NEGATIVE },
	{ "Patch_IncPatchColumn", VK_ADD, RAD_CONTROL | RAD_SHIFT, ID_CURVE_INSERTCOLUMN },
	{ "Patch_IncPatchRow", VK_ADD, RAD_CONTROL, ID_CURVE_INSERTROW },
	{ "Patch_DecPatchColumn", VK_SUBTRACT, RAD_CONTROL | RAD_SHIFT, ID_CURVE_DELETECOLUMN },
	{ "Patch_DecPatchRow", VK_SUBTRACT, RAD_CONTROL, ID_CURVE_DELETEROW },
	{ "Patch_RedisperseRows", 'E', RAD_CONTROL, ID_CURVE_REDISPERSE_ROWS },
	{ "Patch_RedisperseCols", 'E', RAD_CONTROL | RAD_SHIFT, ID_CURVE_REDISPERSE_COLS },
	{ "Patch_Naturalize", 'N', RAD_CONTROL, ID_PATCH_NATURALIZE },
	{ "Patch_SnapToGrid", 'G', RAD_CONTROL, ID_SELECT_SNAPTOGRID },
	{ "Patch_CapCurrentCurve", 'C', RAD_SHIFT, ID_CURVE_CAP },

	{ "Clipper_Toggle", 'X', 0, ID_VIEW_CLIPPER },
	{ "Clipper_ClipSelected", VK_RETURN, 0, ID_CLIP_SELECTED },
	{ "Clipper_SplitSelected", VK_RETURN, RAD_SHIFT, ID_SPLIT_SELECTED },
	{ "Clipper_FlipClip", VK_RETURN, RAD_CONTROL, ID_FLIP_CLIP },

	{ "CameraClip_ZoomOut", VK_OEM_4, RAD_CONTROL, ID_VIEW_CUBEOUT },
	{ "CameraClip_ZoomIn", VK_OEM_5, RAD_CONTROL, ID_VIEW_CUBEIN },
	{ "CameraClip_Toggle", VK_OEM_6, RAD_CONTROL, ID_VIEW_CUBICCLIPPING },

	{ "ViewTab_EntityInfo", 'N', 0, ID_VIEW_ENTITY },
	{ "ViewTab_Console", 'O', 0, ID_VIEW_CONSOLE },
	{ "ViewTab_Textures", 'T', 0, ID_VIEW_TEXTURE },
	{ "ViewTab_MediaBrowser", 'B', 0, ID_INSPECTOR_MEDIABROWSER },

	{ "Window_SurfaceInspector", 'M', 0, ID_TEXTURES_INSPECTOR },
	{ "Window_PatchInspector", 'M', RAD_SHIFT, ID_PATCH_INSPECTOR },
	{ "Window_EntityList", 'I', 0, ID_EDIT_ENTITYINFO },
	{ "Window_Preferences", 'P', 0, ID_PREFS },
	{ "Window_ToggleCamera", 'C', RAD_CONTROL | RAD_SHIFT, ID_TOGGLECAMERA },
	{ "Window_ToggleView", 'V', RAD_CONTROL | RAD_SHIFT, ID_TOGGLEVIEW },
	{ "Window_ToggleZ", 'Z', RAD_CONTROL | RAD_SHIFT, ID_TOGGLEZ },
	{ "Window_LightEditor", 'J', 0, ID_PROJECTED_LIGHT },
	{ "Window_EntityColor", 'K', 0, ID_MISC_SELECTENTITYCOLOR },

	{ "Selection_DragEdges", 'E', 0, ID_SELECTION_DRAGEDGES },
	{ "Selection_DragVertices", 'V', 0, ID_SELECTION_DRAGVERTECIES },
	{ "Selection_Clone", VK_SPACE, 0, ID_SELECTION_CLONE },
	{ "Selection_Delete", VK_BACK, 0, ID_SELECTION_DELETE },
	{ "Selection_UnSelect", VK_ESCAPE, 0, ID_SELECTION_DESELECT },
	{ "Selection_Invert", 'I', 0, ID_SELECTION_INVERT },
	{ "Selection_ToggleMoveOnly", 'W', 0, ID_SELECTION_MOVEONLY },
	{ "Selection_MoveDown", VK_SUBTRACT, 0, ID_SELECTION_MOVEDOWN },
	{ "Selection_MoveUp", VK_ADD, 0, ID_SELECTION_MOVEUP },
	{ "Selection_DumpBrush", 'D', RAD_SHIFT, ID_SELECTION_PRINT },
	{ "Selection_NudgeLeft", VK_LEFT, RAD_ALT, ID_SELECTION_SELECT_NUDGELEFT },
	{ "Selection_NudgeRight", VK_RIGHT, RAD_ALT, ID_SELECTION_SELECT_NUDGERIGHT },
	{ "Selection_NudgeUp", VK_UP, RAD_ALT, ID_SELECTION_SELECT_NUDGEUP },
	{ "Selection_NudgeDown", VK_DOWN, RAD_ALT, ID_SELECTION_SELECT_NUDGEDOWN },
	{ "Selection_Combine", 'K', RAD_SHIFT, ID_SELECTION_COMBINE },
	{ "Selection_Connect", 'K', RAD_CONTROL, ID_SELECTION_CONNECT },
	{ "Selection_Ungroup", 'G', RAD_SHIFT, ID_SELECTION_UNGROUPENTITY },
	{ "Selection_CSGMerge", 'M', RAD_SHIFT, ID_SELECTION_CSGMERGE },
	{ "Selection_CenterOrigin", 'O', RAD_SHIFT, ID_SELECTION_CENTER_ORIGIN },
	{ "Selection_SelectCompleteEntity", 'E', RAD_CONTROL | RAD_ALT | RAD_SHIFT, ID_SELECT_COMPLETE_ENTITY },
	{ "Selection_SelectAllOfType", 'A', RAD_SHIFT, ID_SELECT_ALL },

	{ "Show_ToggleLights", '0', RAD_ALT, ID_VIEW_SHOWLIGHTS },
	{ "Show_TogglePatches", 'P', RAD_CONTROL, ID_VIEW_SHOWCURVES },
	{ "Show_ToggleClip", 'L', RAD_CONTROL, ID_VIEW_SHOWCLIP },
	{ "Show_HideSelected", 'H', 0, ID_VIEW_HIDESHOW_HIDESELECTED },
	{ "Show_ShowHidden", 'H', RAD_SHIFT, ID_VIEW_HIDESHOW_SHOWHIDDEN },
	{ "Show_HideNotSelected", 'H', RAD_CONTROL | RAD_SHIFT, ID_VIEW_HIDESHOW_HIDENOTSELECTED },

	{ "Render_ToggleSound", VK_F9, 0, ID_VIEW_RENDERSOUND },
	{ "Render_ToggleSelections", VK_F8, 0, ID_VIEW_RENDERSELECTION },
	{ "Render_RebuildData", VK_F7, 0, ID_VIEW_REBUILDRENDERDATA },
	{ "Render_ToggleAnimation", VK_F6, 0, ID_VIEW_MATERIALANIMATION },
	{ "Render_ToggleEntityOutlines", VK_F5, 0, ID_VIEW_RENDERENTITYOUTLINES },
	{ "Render_ToggleRealtimeBuild", VK_F4, 0, ID_VIEW_REALTIMEREBUILD },
	{ "Render_Toggle", VK_F3, 0, ID_VIEW_RENDERMODE },

	{ "Find_Textures", 'F', RAD_SHIFT, ID_TEXTURE_REPLACEALL },
	{ "Find_Entity", VK_F3, RAD_CONTROL, ID_MISC_FINDORREPLACEENTITY },
	{ "Find_NextEntity", VK_F3, RAD_SHIFT, ID_MISC_FINDNEXTENT },

	{ "_PlayMap", VK_F2, 0, ID_SHOW_DOOM },
	{ "_CompileMap", VK_F10, 0, ID_DMAP },

	{ "Rotate_MouseRotate", 'R', 0, ID_SELECT_MOUSEROTATE },
	{ "Rotate_ToggleFlatRotation", 'R', RAD_CONTROL, ID_VIEW_CAMERAUPDATE },
	{ "Rotate_CycleRotationAxis", 'R', RAD_SHIFT, ID_TOGGLE_ROTATELOCK },

	{ "_AutoCaulk", 'A', RAD_CONTROL | RAD_SHIFT, ID_AUTOCAULK }, // ctrl-shift-a, since SHIFT-A is already taken
};

int		 g_nCommandCount = sizeof( g_Commands ) / sizeof( SCommandInfo );

SKeyInfo g_Keys[] = {
	/* To understand the VK_* information, please read the MSDN:
		http://msdn.microsoft.com/en-us/library/ms927178.aspx
	*/
	{ "Space", VK_SPACE },
	{ "Backspace", VK_BACK },
	{ "Escape", VK_ESCAPE },
	{ "End", VK_END },
	{ "Insert", VK_INSERT },
	{ "Delete", VK_DELETE },
	{ "PageUp", VK_PRIOR },
	{ "PageDown", VK_NEXT },
	{ "Up", VK_UP },
	{ "Down", VK_DOWN },
	{ "Left", VK_LEFT },
	{ "Right", VK_RIGHT },
	{ "F1", VK_F1 },
	{ "F2", VK_F2 },
	{ "F3", VK_F3 },
	{ "F4", VK_F4 },
	{ "F5", VK_F5 },
	{ "F6", VK_F6 },
	{ "F7", VK_F7 },
	{ "F8", VK_F8 },
	{ "F9", VK_F9 },
	{ "F10", VK_F10 },
	{ "F11", VK_F11 },
	{ "F12", VK_F12 },
	{ "F13", VK_F13 },
	{ "F14", VK_F14 },
	{ "F15", VK_F15 },
	{ "F16", VK_F16 },
	{ "F17", VK_F17 },
	{ "F18", VK_F18 },
	{ "F19", VK_F19 },
	{ "F20", VK_F20 },
	{ "F21", VK_F21 },
	{ "F22", VK_F22 },
	{ "F23", VK_F23 },
	{ "F24", VK_F24 },
	{ "Return", VK_RETURN },
	{ "Comma", VK_COMMA },
	{ "Period", VK_PERIOD },
	{ "Plus", VK_ADD },
	{ "Multiply", VK_MULTIPLY },
	{ "Subtract", VK_SUBTRACT },
	{ "NumPad0", VK_NUMPAD0 },
	{ "NumPad1", VK_NUMPAD1 },
	{ "NumPad2", VK_NUMPAD2 },
	{ "NumPad3", VK_NUMPAD3 },
	{ "NumPad4", VK_NUMPAD4 },
	{ "NumPad5", VK_NUMPAD5 },
	{ "NumPad6", VK_NUMPAD6 },
	{ "NumPad7", VK_NUMPAD7 },
	{ "NumPad8", VK_NUMPAD8 },
	{ "NumPad9", VK_NUMPAD9 },
	{ "[", VK_OEM_4 },	/* Was 219, 0xDB */
	{ "\\", VK_OEM_5 }, /* Was 220, 0xDC */
	{ "]", VK_OEM_6 },	/* Was 221, 0xDD */
};

int	 g_nKeyCount = sizeof( g_Keys ) / sizeof( SKeyInfo );

/*
================
RadiantPrint
================
*/
void RadiantPrint( const char* text )
{
	if( g_editorAlive && g_Inspectors )
	{
		if( g_Inspectors->consoleWnd.GetSafeHwnd() )
		{
			g_Inspectors->consoleWnd.AddText( text );
		}
	}
}

/*
================
RadiantShutdown
================
*/
void RadiantShutdown( void )
{
	theApp.ExitInstance();
}

/*
=================
RadiantInit

This is also called when you 'quit' in doom
=================
*/
void RadiantInit( void )
{
	// make sure the renderer is initialized
	if( !renderSystem->IsOpenGLRunning() )
	{
		common->Printf( "no OpenGL running\n" );
		return;
	}

	g_editorAlive = true;

	// allocate a renderWorld and a soundWorld
	if( g_qeglobals.rw == NULL )
	{
		g_qeglobals.rw = renderSystem->AllocRenderWorld();
		g_qeglobals.rw->InitFromMap( NULL );
	}
	if( g_qeglobals.sw == NULL )
	{
		g_qeglobals.sw = soundSystem->AllocSoundWorld( g_qeglobals.rw );
	}

	if( g_DoomInstance )
	{
		if( ::IsWindowVisible( win32.hWnd ) )
		{
			::ShowWindow( win32.hWnd, SW_HIDE );
			g_pParentWnd->ShowWindow( SW_SHOW );
			g_pParentWnd->SetFocus();
		}
	}
	else
	{
		Sys_GrabMouseCursor( false );

		g_DoomInstance = win32.hInstance;

		InitAfx();

#ifndef _DEBUG
		CSplashScreen::ShowSplashScreen( NULL );
#endif // !_DEBUG

		CWinApp*	pApp	= AfxGetApp();
		CWinThread* pThread = AfxGetThread();

		// App global initializations (rare)
		pApp->InitApplication();

		// Perform specific initializations
		pThread->InitInstance();

		qglFinish();
		// qwglMakeCurrent(0, 0);
		qwglMakeCurrent( win32.hDC, win32.hGLRC );

		// hide the doom window by default
		::ShowWindow( win32.hWnd, SW_HIDE );

#ifndef _DEBUG
		CSplashScreen::HideSplashScreen();
#endif // !_DEBUG
	}
}

/*
================
RadiantRun
================
*/
void RadiantRun( void )
{
	static bool exceptionErr = false;
	int			show		 = ::IsWindowVisible( win32.hWnd );

	try
	{
		if( !exceptionErr && !show )
		{
			qglDepthMask( true );
			theApp.Run();

			if( win32.hDC != NULL && win32.hGLRC != NULL )
			{
				qwglMakeCurrent( win32.hDC, win32.hGLRC );
			}
		}
	}
	catch( idException& ex )
	{
		MessageBoxA( NULL, ex.error, "Exception error", MB_OK );
		RadiantShutdown();
	}
}

/*
=============================================================

REGISTRY INFO

=============================================================
*/

/*
================
SaveRegistryInfo
================
*/
bool SaveRegistryInfo( const char* pszName, void* pvBuf, long lSize )
{
	SetCvarBinary( pszName, pvBuf, lSize );
	common->WriteFlaggedCVarsToFile( "editor.cfg", CVAR_TOOL, "sett" );
	return true;
}

/*
================
LoadRegistryInfo
================
*/
bool LoadRegistryInfo( const char* pszName, void* pvBuf, long* plSize )
{
	return GetCvarBinary( pszName, pvBuf, *plSize );
}

/*
================
SaveWindowState
================
*/
bool SaveWindowState( HWND hWnd, const char* pszName )
{
	RECT rc;
	GetWindowRect( hWnd, &rc );
	if( hWnd != g_pParentWnd->GetSafeHwnd() )
	{
		if( ::GetParent( hWnd ) != g_pParentWnd->GetSafeHwnd() )
		{
			::SetParent( hWnd, g_pParentWnd->GetSafeHwnd() );
		}
		MapWindowPoints( NULL, g_pParentWnd->GetSafeHwnd(), ( POINT* )&rc, 2 );
	}
	return SaveRegistryInfo( pszName, &rc, sizeof( rc ) );
}

/*
================
LoadWindowState
================
*/
bool LoadWindowState( HWND hWnd, const char* pszName )
{
	RECT rc;
	LONG lSize = sizeof( rc );

	if( LoadRegistryInfo( pszName, &rc, &lSize ) )
	{
		if( rc.left < 0 )
		{
			rc.left = 0;
		}

		if( rc.top < 0 )
		{
			rc.top = 0;
		}

		if( rc.right < rc.left + 16 )
		{
			rc.right = rc.left + 16;
		}

		if( rc.bottom < rc.top + 16 )
		{
			rc.bottom = rc.top + 16;
		}

		MoveWindow( hWnd, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, FALSE );
		return true;
	}

	return false;
}

/*
===============================================================

  STATUS WINDOW

===============================================================
*/

/*
================
Sys_UpdateStatusBar
================
*/
void Sys_UpdateStatusBar( void )
{
	extern int g_numbrushes, g_numentities;

	char	   numbrushbuffer[100] = "";

	sprintf( numbrushbuffer, "Brushes: %d Entities: %d", g_numbrushes, g_numentities );
	Sys_Status( numbrushbuffer, 2 );
}

/*
================
Sys_Status
================
*/
void Sys_Status( const char* psz, int part )
{
	if( part < 0 )
	{
		common->Printf( "%s", psz );
		part = 0;
	}

	g_pParentWnd->SetStatusText( part, psz );
}
