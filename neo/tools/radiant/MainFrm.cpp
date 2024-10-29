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

#define IDSTR_NO_REDIRECT

#include "precompiled.h"
#pragma hdrstop

#include "qe3.h"
#include "Radiant.h"
#include "ZWnd.h"
#include "CamWnd.h"
#include "MapInfo.h"
#include "MainFrm.h"
#include "RotateDlg.h"
#include "EntityListDlg.h"
#include "CommandsDlg.h"
#include "ScaleDialog.h"
#include "FindTextureDlg.h"
#include "SurfaceDlg.h"
#include "shlobj.h"
#include "DialogTextures.h"
#include "PatchDensityDlg.h"
#include "DialogThick.h"
#include "PatchDialog.h"
#include "Undo.h"
#include "NewTexWnd.h"
#include "splines.h"
#include "dlgcamera.h"
#include "mmsystem.h"
#include "LightDlg.h"
#include "GetString.h"
#include "EntKeyFindReplace.h"
#include "InspectorDialog.h"
#include "autocaulk.h"

#include "../../sys/win32/rc/resource.h"
#include "../comafx/DialogName.h"
#include "../comafx/DialogColorPicker.h"

#ifdef _DEBUG
	#define new DEBUG_NEW
#endif

#undef min

// globals
CString			g_strAppPath;						// holds the full path of the executable
CMainFrame*		g_pParentWnd = NULL;				// used to precast to CMainFrame
CPrefsDlg		g_Preferences;						// global prefs instance
CPrefsDlg&		g_PrefsDlg = g_Preferences;		// reference used throughout
bool			g_bScreenUpdates = true;			// whether window painting is active, used in a few places


#define SPEED_MOVE	32.0f
#define SPEED_TURN	22.5f

#define MAX_GRID	64.0f
#define MIN_GRID	0.125f

static void SetToolbarButtonCheckState( CMFCToolBar& toolbar, UINT buttonID, BOOL check )
{
	if( toolbar.GetSafeHwnd() )
	{
		int nIndex = toolbar.CommandToIndex( buttonID );
		if( nIndex >= 0 )
		{
			UINT nStyle = toolbar.GetButtonStyle( nIndex );
			if( check )
			{
				nStyle |= TBBS_CHECKED; // Check the button
			}
			else
			{
				nStyle &= ~TBBS_CHECKED; // Uncheck the button
			}
			toolbar.SetButtonStyle( nIndex, nStyle );
			toolbar.Invalidate(); // Redraw the toolbar to reflect the change
		}
	}
}


const int		IDMRU_END = IDMRU + 9;

IMPLEMENT_DYNAMIC( CMainFrame, CFrameWndEx )
BEGIN_MESSAGE_MAP( CMainFrame, CFrameWndEx )
	ON_WM_CREATE()
	ON_WM_TIMER()
	ON_WM_DESTROY()
	ON_WM_CLOSE()
	ON_WM_KEYDOWN()
	ON_WM_SIZE()
	ON_COMMAND( ID_VIEW_CAMERATOGGLE, &CMainFrame::ToggleCamera )
	ON_COMMAND( ID_FILE_EXIT, &CMainFrame::OnFileExit )
	ON_COMMAND( ID_FILE_NEW, &CMainFrame::OnFileNew )
	ON_COMMAND( ID_FILE_OPEN, &CMainFrame::OnFileOpen )
	ON_COMMAND( ID_FILE_POINTFILE, &CMainFrame::OnFilePointfile )
	ON_COMMAND( ID_FILE_SAVE, &CMainFrame::OnFileSave )
	ON_COMMAND( ID_FILE_SAVEAS, &CMainFrame::OnFileSaveas )
	ON_COMMAND( ID_SHOW_MODELS, &CMainFrame::OnViewShowModels )
	ON_COMMAND( ID_VIEW_CONSOLE, &CMainFrame::OnInspectorConsole )
	ON_COMMAND( ID_VIEW_ENTITY, &CMainFrame::OnInspectorEntity )
	ON_COMMAND( ID_INSPECTOR_MEDIABROWSER, &CMainFrame::OnInspectorMediaBrowser )
	ON_COMMAND( ID_VIEW_TEXTURE, &CMainFrame::OnInspectorTexture )
	ON_COMMAND( ID_VIEW_INSPECTOR, &CMainFrame::OnViewInspector )
	ON_COMMAND( ID_VIEW_100, &CMainFrame::OnView100 )
	ON_COMMAND( ID_VIEW_CENTER, &CMainFrame::OnViewCenter )
	ON_COMMAND( ID_VIEW_DOWNFLOOR, &CMainFrame::OnViewDownfloor )
	ON_COMMAND( ID_VIEW_FRONT, &CMainFrame::OnViewFront )
	ON_COMMAND( ID_VIEW_SHOWBLOCKS, &CMainFrame::OnViewShowblocks )
	ON_COMMAND( ID_VIEW_SHOWCLIP, &CMainFrame::OnViewShowclip )
	ON_COMMAND( ID_VIEW_SHOWTRIGGERS, &CMainFrame::OnViewShowTriggers )
	ON_COMMAND( ID_VIEW_SHOWCOORDINATES, &CMainFrame::OnViewShowcoordinates )
	ON_COMMAND( ID_VIEW_SHOWENT, &CMainFrame::OnViewShowent )
	ON_COMMAND( ID_VIEW_SHOWLIGHTS, &CMainFrame::OnViewShowlights )
	ON_COMMAND( ID_VIEW_SHOWNAMES, &CMainFrame::OnViewShownames )
	ON_COMMAND( ID_VIEW_SHOWPATH, &CMainFrame::OnViewShowpath )
	ON_COMMAND( ID_VIEW_SHOWCOMBATNODES, &CMainFrame::OnViewShowCombatNodes )
	ON_COMMAND( ID_VIEW_SHOWWATER, &CMainFrame::OnViewShowwater )
	ON_COMMAND( ID_VIEW_SHOWWORLD, &CMainFrame::OnViewShowworld )
	ON_COMMAND( ID_VIEW_UPFLOOR, &CMainFrame::OnViewUpfloor )
	ON_COMMAND( ID_VIEW_XY, &CMainFrame::OnViewXy )
	ON_COMMAND( ID_VIEW_Z100, &CMainFrame::OnViewZ100 )
	ON_COMMAND( ID_VIEW_ZOOMIN, &CMainFrame::OnViewZoomin )
	ON_COMMAND( ID_VIEW_ZOOMOUT, &CMainFrame::OnViewZoomout )
	ON_COMMAND( ID_VIEW_ZZOOMIN, &CMainFrame::OnViewZzoomin )
	ON_COMMAND( ID_VIEW_ZZOOMOUT, &CMainFrame::OnViewZzoomout )
	ON_COMMAND( ID_VIEW_SIDE, &CMainFrame::OnViewSide )
	ON_COMMAND( ID_TEXTURES_SHOWINUSE, &CMainFrame::OnTexturesShowinuse )
	ON_COMMAND( ID_TEXTURES_INSPECTOR, &CMainFrame::OnTexturesInspector )
	ON_COMMAND( ID_MISC_FINDBRUSH, &CMainFrame::OnMiscFindbrush )
	ON_COMMAND( ID_MISC_NEXTLEAKSPOT, &CMainFrame::OnMiscNextleakspot )
	ON_COMMAND( ID_MISC_PREVIOUSLEAKSPOT, &CMainFrame::OnMiscPreviousleakspot )
	ON_COMMAND( ID_MISC_SELECTENTITYCOLOR, &CMainFrame::OnMiscSelectentitycolor )
	ON_COMMAND( ID_MISC_FINDORREPLACEENTITY, &CMainFrame::OnMiscFindOrReplaceEntity )
	ON_COMMAND( ID_MISC_FINDNEXTENT, &CMainFrame::OnMiscFindNextEntity )
	ON_COMMAND( ID_TEXTUREBK, &CMainFrame::OnTexturebk )
	ON_COMMAND( ID_COLORS_MAJOR, &CMainFrame::OnColorsMajor )
	ON_COMMAND( ID_COLORS_MINOR, &CMainFrame::OnColorsMinor )
	ON_COMMAND( ID_COLORS_XYBK, &CMainFrame::OnColorsXybk )
	ON_COMMAND( ID_DMAP, &CMainFrame::OnCompileMap )
	ON_COMMAND( ID_BRUSH_3SIDED, &CMainFrame::OnBrush3sided )
	ON_COMMAND( ID_BRUSH_4SIDED, &CMainFrame::OnBrush4sided )
	ON_COMMAND( ID_BRUSH_5SIDED, &CMainFrame::OnBrush5sided )
	ON_COMMAND( ID_BRUSH_6SIDED, &CMainFrame::OnBrush6sided )
	ON_COMMAND( ID_BRUSH_7SIDED, &CMainFrame::OnBrush7sided )
	ON_COMMAND( ID_BRUSH_8SIDED, &CMainFrame::OnBrush8sided )
	ON_COMMAND( ID_BRUSH_9SIDED, &CMainFrame::OnBrush9sided )
	ON_COMMAND( ID_BRUSH_ARBITRARYSIDED, &CMainFrame::OnBrushArbitrarysided )
	ON_COMMAND( ID_BRUSH_FLIPX, &CMainFrame::OnBrushFlipx )
	ON_COMMAND( ID_BRUSH_FLIPY, &CMainFrame::OnBrushFlipy )
	ON_COMMAND( ID_BRUSH_FLIPZ, &CMainFrame::OnBrushFlipz )
	ON_COMMAND( ID_BRUSH_ROTATEX, &CMainFrame::OnBrushRotatex )
	ON_COMMAND( ID_BRUSH_ROTATEY, &CMainFrame::OnBrushRotatey )
	ON_COMMAND( ID_BRUSH_ROTATEZ, &CMainFrame::OnBrushRotatez )
	ON_COMMAND( ID_REGION_OFF, &CMainFrame::OnRegionOff )
	ON_COMMAND( ID_REGION_SETBRUSH, &CMainFrame::OnRegionSetbrush )
	ON_COMMAND( ID_REGION_SETSELECTION, &CMainFrame::OnRegionSetselection )
	ON_COMMAND( ID_REGION_SETTALLBRUSH, &CMainFrame::OnRegionSettallbrush )
	ON_COMMAND( ID_REGION_SETXY, &CMainFrame::OnRegionSetxy )
	ON_COMMAND( ID_SELECTION_ARBITRARYROTATION, &CMainFrame::OnSelectionArbitraryrotation )
	ON_COMMAND( ID_SELECTION_CLONE, &CMainFrame::OnSelectionClone )
	ON_COMMAND( ID_SELECTION_CONNECT, &CMainFrame::OnSelectionConnect )
	ON_COMMAND( ID_SELECTION_CSGSUBTRACT, &CMainFrame::OnSelectionCsgsubtract )
	ON_COMMAND( ID_SELECTION_CSGMERGE, &CMainFrame::OnSelectionCsgmerge )
	ON_COMMAND( ID_SELECTION_DELETE, &CMainFrame::OnSelectionDelete )
	ON_COMMAND( ID_SELECTION_DESELECT, &CMainFrame::OnSelectionDeselect )
	ON_COMMAND( ID_SELECTION_DRAGEDGES, &CMainFrame::OnSelectionDragedges )
	ON_COMMAND( ID_SELECTION_DRAGVERTECIES, &CMainFrame::OnSelectionDragvertecies )
	ON_COMMAND( ID_SELECTION_CENTER_ORIGIN, &CMainFrame::OnSelectionCenterOrigin )
	ON_COMMAND( ID_SELECTION_MAKEHOLLOW, &CMainFrame::OnSelectionMakehollow )
	ON_COMMAND( ID_SELECTION_SELECTCOMPLETETALL, &CMainFrame::OnSelectionSelectcompletetall )
	ON_COMMAND( ID_SELECTION_SELECTINSIDE, &CMainFrame::OnSelectionSelectinside )
	ON_COMMAND( ID_SELECTION_SELECTPARTIALTALL, &CMainFrame::OnSelectionSelectpartialtall )
	ON_COMMAND( ID_SELECTION_SELECTTOUCHING, &CMainFrame::OnSelectionSelecttouching )
	ON_COMMAND( ID_SELECTION_UNGROUPENTITY, &CMainFrame::OnSelectionUngroupentity )
	ON_COMMAND( ID_TEXTURES_POPUP, &CMainFrame::OnTexturesPopup )
	ON_COMMAND( ID_SPLINES_POPUP, &CMainFrame::OnSplinesPopup )
	ON_COMMAND( ID_SPLINES_EDITPOINTS, &CMainFrame::OnSplinesEditPoints )
	ON_COMMAND( ID_SPLINES_ADDPOINTS, &CMainFrame::OnSplinesAddPoints )
	ON_COMMAND( ID_SPLINES_INSERTPOINTS, &CMainFrame::OnSplinesInsertPoint )
	ON_COMMAND( ID_SPLINES_DELETEPOINTS, &CMainFrame::OnSplinesDeletePoint )
	ON_COMMAND( ID_POPUP_SELECTION, &CMainFrame::OnPopupSelection )
	ON_COMMAND( ID_VIEW_CHANGE, &CMainFrame::OnViewChange )
	ON_COMMAND( ID_VIEW_CAMERAUPDATE, &CMainFrame::OnViewCameraupdate )
	ON_WM_SIZING()
	ON_COMMAND( ID_VIEW_CLIPPER, &CMainFrame::OnViewClipper )
	ON_COMMAND( ID_CAMERA_ANGLEDOWN, &CMainFrame::OnCameraAngledown )
	ON_COMMAND( ID_CAMERA_ANGLEUP, &CMainFrame::OnCameraAngleup )
	ON_COMMAND( ID_CAMERA_BACK, &CMainFrame::OnCameraBack )
	ON_COMMAND( ID_CAMERA_DOWN, &CMainFrame::OnCameraDown )
	ON_COMMAND( ID_CAMERA_FORWARD, &CMainFrame::OnCameraForward )
	ON_COMMAND( ID_CAMERA_LEFT, &CMainFrame::OnCameraLeft )
	ON_COMMAND( ID_CAMERA_RIGHT, &CMainFrame::OnCameraRight )
	ON_COMMAND( ID_CAMERA_STRAFELEFT, &CMainFrame::OnCameraStrafeleft )
	ON_COMMAND( ID_CAMERA_STRAFERIGHT, &CMainFrame::OnCameraStraferight )
	ON_COMMAND( ID_CAMERA_UP, &CMainFrame::OnCameraUp )
	ON_COMMAND( ID_GRID_TOGGLE, &CMainFrame::OnGridToggle )
	ON_COMMAND( ID_PREFS, &CMainFrame::OnPrefs )
	ON_COMMAND( ID_TOGGLECAMERA, &CMainFrame::OnTogglecamera )
	ON_COMMAND( ID_TOGGLEVIEW, &CMainFrame::OnToggleview )
	ON_COMMAND( ID_TOGGLEZ, &CMainFrame::OnTogglez )
	ON_COMMAND( ID_TOGGLE_LOCK, &CMainFrame::OnToggleLock )
	ON_COMMAND( ID_EDIT_MAPINFO, &CMainFrame::OnEditMapinfo )
	ON_COMMAND( ID_EDIT_ENTITYINFO, &CMainFrame::OnEditEntityinfo )
	ON_COMMAND( ID_VIEW_NEXTVIEW, &CMainFrame::OnViewNextview )
	ON_COMMAND( ID_HELP_COMMANDLIST, &CMainFrame::OnHelpCommandlist )
	ON_COMMAND( ID_FLIP_CLIP, &CMainFrame::OnFlipClip )
	ON_COMMAND( ID_CLIP_SELECTED, &CMainFrame::OnClipSelected )
	ON_COMMAND( ID_SPLIT_SELECTED, &CMainFrame::OnSplitSelected )
	ON_COMMAND( ID_TOGGLEVIEW_XZ, &CMainFrame::OnToggleviewXz )
	ON_COMMAND( ID_TOGGLEVIEW_YZ, &CMainFrame::OnToggleviewYz )
	ON_COMMAND( ID_COLORS_BRUSH, &CMainFrame::OnColorsBrush )
	ON_COMMAND( ID_COLORS_CLIPPER, &CMainFrame::OnColorsClipper )
	ON_COMMAND( ID_COLORS_GRIDTEXT, &CMainFrame::OnColorsGridtext )
	ON_COMMAND( ID_COLORS_SELECTEDBRUSH, &CMainFrame::OnColorsSelectedbrush )
	ON_COMMAND( ID_COLORS_GRIDBLOCK, &CMainFrame::OnColorsGridblock )
	ON_COMMAND( ID_COLORS_VIEWNAME, &CMainFrame::OnColorsViewname )
	ON_COMMAND( ID_COLOR_SETORIGINAL, &CMainFrame::OnColorSetoriginal )
	ON_COMMAND( ID_COLOR_SETQER, &CMainFrame::OnColorSetqer )
	ON_COMMAND( ID_COLOR_SUPERMAL, &CMainFrame::OnColorSetSuperMal )
	ON_COMMAND( ID_THEMES_MAX , &CMainFrame::OnColorSetMax )
	ON_COMMAND( ID_COLOR_SETBLACK, &CMainFrame::OnColorSetblack )
	ON_COMMAND( ID_SNAPTOGRID, &CMainFrame::OnSnaptogrid )
	ON_COMMAND( ID_SELECT_SCALE, &CMainFrame::OnSelectScale )
	ON_COMMAND( ID_SELECT_MOUSEROTATE, &CMainFrame::OnSelectMouserotate )
	ON_COMMAND( ID_EDIT_COPYBRUSH, &CMainFrame::OnEditCopybrush )
	ON_COMMAND( ID_EDIT_PASTEBRUSH, &CMainFrame::OnEditPastebrush )
	ON_COMMAND( ID_EDIT_UNDO, &CMainFrame::OnEditUndo )
	ON_COMMAND( ID_EDIT_REDO, &CMainFrame::OnEditRedo )
	ON_UPDATE_COMMAND_UI( ID_EDIT_UNDO, &CMainFrame::OnUpdateEditUndo )
	ON_UPDATE_COMMAND_UI( ID_EDIT_REDO, &CMainFrame::OnUpdateEditRedo )
	ON_COMMAND( ID_SELECTION_INVERT, &CMainFrame::OnSelectionInvert )
	ON_COMMAND( ID_SELECTION_TEXTURE_DEC, &CMainFrame::OnSelectionTextureDec )
	ON_COMMAND( ID_SELECTION_TEXTURE_FIT, &CMainFrame::OnSelectionTextureFit )
	ON_COMMAND( ID_SELECTION_TEXTURE_INC, &CMainFrame::OnSelectionTextureInc )
	ON_COMMAND( ID_SELECTION_TEXTURE_ROTATECLOCK, &CMainFrame::OnSelectionTextureRotateclock )
	ON_COMMAND( ID_SELECTION_TEXTURE_ROTATECOUNTER, &CMainFrame::OnSelectionTextureRotatecounter )
	ON_COMMAND( ID_SELECTION_TEXTURE_SCALEDOWN, &CMainFrame::OnSelectionTextureScaledown )
	ON_COMMAND( ID_SELECTION_TEXTURE_SCALEUP, &CMainFrame::OnSelectionTextureScaleup )
	ON_COMMAND( ID_SELECTION_TEXTURE_SHIFTDOWN, &CMainFrame::OnSelectionTextureShiftdown )
	ON_COMMAND( ID_SELECTION_TEXTURE_SHIFTLEFT, &CMainFrame::OnSelectionTextureShiftleft )
	ON_COMMAND( ID_SELECTION_TEXTURE_SHIFTRIGHT, &CMainFrame::OnSelectionTextureShiftright )
	ON_COMMAND( ID_SELECTION_TEXTURE_SHIFTUP, &CMainFrame::OnSelectionTextureShiftup )
	ON_COMMAND( ID_GRID_NEXT, &CMainFrame::OnGridNext )
	ON_COMMAND( ID_GRID_PREV, &CMainFrame::OnGridPrev )
	ON_COMMAND( ID_SELECTION_TEXTURE_SCALELEFT, &CMainFrame::OnSelectionTextureScaleLeft )
	ON_COMMAND( ID_SELECTION_TEXTURE_SCALERIGHT, &CMainFrame::OnSelectionTextureScaleRight )
	ON_COMMAND( ID_TEXTURE_REPLACEALL, &CMainFrame::OnTextureReplaceall )
	ON_COMMAND( ID_SCALELOCKX, &CMainFrame::OnScalelockx )
	ON_COMMAND( ID_SCALELOCKY, &CMainFrame::OnScalelocky )
	ON_COMMAND( ID_SCALELOCKZ, &CMainFrame::OnScalelockz )
	ON_COMMAND( ID_SELECT_MOUSESCALE, &CMainFrame::OnSelectMousescale )
	ON_COMMAND( ID_VIEW_CUBICCLIPPING, &CMainFrame::OnViewCubicclipping )
	ON_COMMAND( ID_FILE_IMPORT, &CMainFrame::OnFileImport )
	ON_UPDATE_COMMAND_UI( ID_FILE_IMPORT, &CMainFrame::OnUpdateFileImport )
	ON_COMMAND( ID_VIEW_CUBEIN, &CMainFrame::OnViewCubein )
	ON_COMMAND( ID_VIEW_CUBEOUT, &CMainFrame::OnViewCubeout )
	ON_COMMAND( ID_FILE_SAVEREGION, &CMainFrame::OnFileSaveregion )
	ON_UPDATE_COMMAND_UI( ID_FILE_SAVEREGION, &CMainFrame::OnUpdateFileSaveregion )
	ON_COMMAND( ID_SELECTION_MOVEDOWN, &CMainFrame::OnSelectionMovedown )
	ON_COMMAND( ID_SELECTION_MOVEUP, &CMainFrame::OnSelectionMoveup )
	ON_COMMAND( ID_SELECTION_PRINT, &CMainFrame::OnSelectionPrint )
	ON_COMMAND( ID_SELECTION_TOGGLESIZEPAINT, &CMainFrame::OnSelectionTogglesizepaint )
	ON_COMMAND( ID_BRUSH_MAKECONE, &CMainFrame::OnBrushMakecone )
	ON_COMMAND( ID_TEXTURES_LOAD, &CMainFrame::OnTexturesLoad )
	ON_COMMAND( ID_TOGGLE_ROTATELOCK, &CMainFrame::OnToggleRotatelock )
	ON_COMMAND( ID_CURVE_BEVEL, &CMainFrame::OnCurveBevel )
	ON_COMMAND( ID_CURVE_INCREASE_VERT, &CMainFrame::OnCurveIncreaseVert )
	ON_COMMAND( ID_CURVE_DECREASE_VERT, &CMainFrame::OnCurveDecreaseVert )
	ON_COMMAND( ID_CURVE_INCREASE_HORZ, &CMainFrame::OnCurveIncreaseHorz )
	ON_COMMAND( ID_CURVE_DECREASE_HORZ, &CMainFrame::OnCurveDecreaseHorz )
	ON_COMMAND( ID_CURVE_CYLINDER, &CMainFrame::OnCurveCylinder )
	ON_COMMAND( ID_CURVE_EIGHTHSPHERE, &CMainFrame::OnCurveEighthsphere )
	ON_COMMAND( ID_CURVE_ENDCAP, &CMainFrame::OnCurveEndcap )
	ON_COMMAND( ID_CURVE_HEMISPHERE, &CMainFrame::OnCurveHemisphere )
	ON_COMMAND( ID_CURVE_INVERTCURVE, &CMainFrame::OnCurveInvertcurve )
	ON_COMMAND( ID_CURVE_QUARTER, &CMainFrame::OnCurveQuarter )
	ON_COMMAND( ID_CURVE_SPHERE, &CMainFrame::OnCurveSphere )
	ON_COMMAND( ID_FILE_IMPORTMAP, &CMainFrame::OnFileImportmap )
	ON_COMMAND( ID_FILE_EXPORTMAP, &CMainFrame::OnFileExportmap )
	ON_COMMAND( ID_EDIT_LOADPREFAB, &CMainFrame::OnEditLoadprefab )
	ON_COMMAND( ID_VIEW_SHOWCURVES, &CMainFrame::OnViewShowcurves )
	ON_COMMAND( ID_SELECTION_SELECT_NUDGEDOWN, &CMainFrame::OnSelectionSelectNudgedown )
	ON_COMMAND( ID_SELECTION_SELECT_NUDGELEFT, &CMainFrame::OnSelectionSelectNudgeleft )
	ON_COMMAND( ID_SELECTION_SELECT_NUDGERIGHT, &CMainFrame::OnSelectionSelectNudgeright )
	ON_COMMAND( ID_SELECTION_SELECT_NUDGEUP, &CMainFrame::OnSelectionSelectNudgeup )
	ON_WM_SYSKEYDOWN()
	ON_COMMAND( ID_TEXTURES_LOADLIST, &CMainFrame::OnTexturesLoadlist )
	ON_COMMAND( ID_DYNAMIC_LIGHTING, &CMainFrame::OnDynamicLighting )
	ON_COMMAND( ID_CURVE_SIMPLEPATCHMESH, &CMainFrame::OnCurveSimplepatchmesh )
	ON_COMMAND( ID_PATCH_SHOWBOUNDINGBOX, &CMainFrame::OnPatchToggleBox )
	ON_COMMAND( ID_PATCH_WIREFRAME, &CMainFrame::OnPatchWireframe )
	ON_COMMAND( ID_CURVE_PATCHCONE, &CMainFrame::OnCurvePatchcone )
	ON_COMMAND( ID_CURVE_PATCHTUBE, &CMainFrame::OnCurvePatchtube )
	ON_COMMAND( ID_PATCH_WELD, &CMainFrame::OnPatchWeld )
	ON_COMMAND( ID_CURVE_PATCHBEVEL, &CMainFrame::OnCurvePatchbevel )
	ON_COMMAND( ID_CURVE_PATCHENDCAP, &CMainFrame::OnCurvePatchendcap )
	ON_COMMAND( ID_CURVE_PATCHINVERTEDBEVEL, &CMainFrame::OnCurvePatchinvertedbevel )
	ON_COMMAND( ID_CURVE_PATCHINVERTEDENDCAP, &CMainFrame::OnCurvePatchinvertedendcap )
	ON_COMMAND( ID_PATCH_DRILLDOWN, &CMainFrame::OnPatchDrilldown )
	ON_COMMAND( ID_CURVE_INSERTCOLUMN, &CMainFrame::OnCurveInsertcolumn )
	ON_COMMAND( ID_CURVE_INSERTROW, &CMainFrame::OnCurveInsertrow )
	ON_COMMAND( ID_CURVE_DELETECOLUMN, &CMainFrame::OnCurveDeletecolumn )
	ON_COMMAND( ID_CURVE_DELETEROW, &CMainFrame::OnCurveDeleterow )
	ON_COMMAND( ID_CURVE_INSERT_ADDCOLUMN, &CMainFrame::OnCurveInsertAddcolumn )
	ON_COMMAND( ID_CURVE_INSERT_ADDROW, &CMainFrame::OnCurveInsertAddrow )
	ON_COMMAND( ID_CURVE_INSERT_INSERTCOLUMN, &CMainFrame::OnCurveInsertInsertcolumn )
	ON_COMMAND( ID_CURVE_INSERT_INSERTROW, &CMainFrame::OnCurveInsertInsertrow )
	ON_COMMAND( ID_CURVE_NEGATIVE, &CMainFrame::OnCurveNegative )
	ON_COMMAND( ID_CURVE_NEGATIVETEXTUREX, &CMainFrame::OnCurveNegativeTextureX )
	ON_COMMAND( ID_CURVE_NEGATIVETEXTUREY, &CMainFrame::OnCurveNegativeTextureY )
	ON_COMMAND( ID_CURVE_DELETE_FIRSTCOLUMN, &CMainFrame::OnCurveDeleteFirstcolumn )
	ON_COMMAND( ID_CURVE_DELETE_FIRSTROW, &CMainFrame::OnCurveDeleteFirstrow )
	ON_COMMAND( ID_CURVE_DELETE_LASTCOLUMN, &CMainFrame::OnCurveDeleteLastcolumn )
	ON_COMMAND( ID_CURVE_DELETE_LASTROW, &CMainFrame::OnCurveDeleteLastrow )
	ON_COMMAND( ID_PATCH_BEND, &CMainFrame::OnPatchBend )
	ON_COMMAND( ID_PATCH_INSDEL, &CMainFrame::OnPatchInsdel )
	ON_COMMAND( ID_PATCH_ENTER, &CMainFrame::OnPatchEnter )
	ON_COMMAND( ID_PATCH_TAB, &CMainFrame::OnPatchTab )
	ON_COMMAND( ID_CURVE_PATCHDENSETUBE, &CMainFrame::OnCurvePatchdensetube )
	ON_COMMAND( ID_CURVE_PATCHVERYDENSETUBE, &CMainFrame::OnCurvePatchverydensetube )
	ON_COMMAND( ID_CURVE_CAP, &CMainFrame::OnCurveCap )
	ON_COMMAND( ID_CURVE_CAP_INVERTEDBEVEL, &CMainFrame::OnCurveCapInvertedbevel )
	ON_COMMAND( ID_CURVE_CAP_INVERTEDENDCAP, &CMainFrame::OnCurveCapInvertedendcap )
	ON_COMMAND( ID_CURVE_REDISPERSE_COLS, &CMainFrame::OnCurveRedisperseCols )
	ON_COMMAND( ID_CURVE_REDISPERSE_ROWS, &CMainFrame::OnCurveRedisperseRows )
	ON_COMMAND( ID_PATCH_NATURALIZE, &CMainFrame::OnPatchNaturalize )
	ON_COMMAND( ID_PATCH_NATURALIZEALT, &CMainFrame::OnPatchNaturalizeAlt )
	ON_COMMAND( ID_SELECT_SNAPTOGRID, &CMainFrame::OnSnapToGrid )
	ON_COMMAND( ID_CURVE_PATCHSQUARE, &CMainFrame::OnCurvePatchsquare )
	ON_COMMAND( ID_TEXTURES_TEXTUREWINDOWSCALE_10, &CMainFrame::OnTexturesTexturewindowscale10 )
	ON_COMMAND( ID_TEXTURES_TEXTUREWINDOWSCALE_100, &CMainFrame::OnTexturesTexturewindowscale100 )
	ON_COMMAND( ID_TEXTURES_TEXTUREWINDOWSCALE_200, &CMainFrame::OnTexturesTexturewindowscale200 )
	ON_COMMAND( ID_TEXTURES_TEXTUREWINDOWSCALE_25, &CMainFrame::OnTexturesTexturewindowscale25 )
	ON_COMMAND( ID_TEXTURES_TEXTUREWINDOWSCALE_50, &CMainFrame::OnTexturesTexturewindowscale50 )
	ON_COMMAND( ID_TEXTURES_FLUSH, &CMainFrame::OnTexturesFlush )
	ON_COMMAND( ID_CURVE_OVERLAY_CLEAR, &CMainFrame::OnCurveOverlayClear )
	ON_COMMAND( ID_CURVE_OVERLAY_SET, &CMainFrame::OnCurveOverlaySet )
	ON_COMMAND( ID_CURVE_THICKEN, &CMainFrame::OnCurveThicken )
	ON_COMMAND( ID_CURVE_CYCLECAP, &CMainFrame::OnCurveCyclecap )
	ON_COMMAND( ID_CURVE_CYCLECAPALT, &CMainFrame::OnCurveCyclecapAlt )
	ON_COMMAND( ID_CURVE_MATRIX_TRANSPOSE, &CMainFrame::OnCurveMatrixTranspose )
	ON_COMMAND( ID_TEXTURES_RELOADSHADERS, &CMainFrame::OnTexturesReloadshaders )
	ON_COMMAND( ID_SHOW_ENTITIES, &CMainFrame::OnShowEntities )
	ON_COMMAND( ID_VIEW_ENTITIESAS_SKINNED, &CMainFrame::OnViewEntitiesasSkinned )
	ON_COMMAND( ID_VIEW_ENTITIESAS_WIREFRAME, &CMainFrame::OnViewEntitiesasWireframe )
	ON_COMMAND( ID_VIEW_SHOWHINT, &CMainFrame::OnViewShowhint )
	ON_UPDATE_COMMAND_UI( ID_TEXTURES_SHOWINUSE, &CMainFrame::OnUpdateTexturesShowinuse )
	ON_COMMAND( ID_TEXTURES_SHOWALL, &CMainFrame::OnTexturesShowall )
	ON_COMMAND( ID_TEXTURES_HIDEALL, &CMainFrame::OnTexturesHideall )
	ON_COMMAND( ID_PATCH_INSPECTOR, &CMainFrame::OnPatchInspector )
	ON_COMMAND( ID_VIEW_OPENGLLIGHTING, &CMainFrame::OnViewOpengllighting )
	ON_COMMAND( ID_SELECT_ALL, &CMainFrame::OnSelectAll )
	ON_COMMAND( ID_VIEW_SHOWCAULK, &CMainFrame::OnViewShowcaulk )
	ON_COMMAND( ID_CURVE_FREEZE, &CMainFrame::OnCurveFreeze )
	ON_COMMAND( ID_CURVE_UNFREEZE, &CMainFrame::OnCurveUnFreeze )
	ON_COMMAND( ID_CURVE_UNFREEZEALL, &CMainFrame::OnCurveUnFreezeAll )
	ON_COMMAND( ID_SELECT_RESELECT, &CMainFrame::OnSelectReselect )
	ON_COMMAND( ID_VIEW_SHOWANGLES, &CMainFrame::OnViewShowangles )
	ON_COMMAND( ID_EDIT_SAVEPREFAB, &CMainFrame::OnEditSaveprefab )
	ON_COMMAND( ID_CURVE_MOREENDCAPSBEVELS_SQUAREBEVEL, &CMainFrame::OnCurveMoreendcapsbevelsSquarebevel )
	ON_COMMAND( ID_CURVE_MOREENDCAPSBEVELS_SQUAREENDCAP, &CMainFrame::OnCurveMoreendcapsbevelsSquareendcap )
	ON_COMMAND( ID_BRUSH_PRIMITIVES_SPHERE, &CMainFrame::OnBrushPrimitivesSphere )
	ON_COMMAND( ID_VIEW_CROSSHAIR, &CMainFrame::OnViewCrosshair )
	ON_COMMAND( ID_VIEW_HIDESHOW_HIDESELECTED, &CMainFrame::OnViewHideshowHideselected )
	ON_COMMAND( ID_VIEW_HIDESHOW_HIDENOTSELECTED, &CMainFrame::OnViewHideshowHideNotselected )
	ON_COMMAND( ID_VIEW_HIDESHOW_SHOWHIDDEN, &CMainFrame::OnViewHideshowShowhidden )
	ON_COMMAND( ID_TEXTURES_SHADERS_SHOW, &CMainFrame::OnTexturesShadersShow )
	ON_COMMAND( ID_TEXTURES_FLUSH_UNUSED, &CMainFrame::OnTexturesFlushUnused )
	ON_COMMAND( ID_PROJECTED_LIGHT, &CMainFrame::OnProjectedLight )
	ON_COMMAND( ID_SHOW_LIGHTTEXTURES, &CMainFrame::OnShowLighttextures )
	ON_COMMAND( ID_SHOW_LIGHTVOLUMES, &CMainFrame::OnShowLightvolumes )
	ON_WM_ACTIVATE()
	ON_COMMAND( ID_SPLINES_MODE, &CMainFrame::OnSplinesMode )
	ON_COMMAND( ID_SPLINES_LOAD, &CMainFrame::OnSplinesLoad )
	ON_COMMAND( ID_SPLINES_SAVE, &CMainFrame::OnSplinesSave )
	//ON_COMMAND(ID_SPLINES_EDIT, &CMainFrame::OnSplinesEdit)
	ON_COMMAND( ID_SPLINE_TEST, &CMainFrame::OnSplineTest )
	ON_COMMAND( ID_POPUP_NEWCAMERA_INTERPOLATED, &CMainFrame::OnPopupNewcameraInterpolated )
	ON_COMMAND( ID_POPUP_NEWCAMERA_SPLINE, &CMainFrame::OnPopupNewcameraSpline )
	ON_COMMAND( ID_POPUP_NEWCAMERA_FIXED, &CMainFrame::OnPopupNewcameraFixed )
	ON_COMMAND( ID_SELECTION_MOVEONLY, &CMainFrame::OnSelectionMoveonly )
	ON_COMMAND( ID_SELECT_BRUSHESONLY, &CMainFrame::OnSelectBrushesOnly )
	ON_COMMAND( ID_SELECT_BYBOUNDINGBRUSH, &CMainFrame::OnSelectByBoundingBrush )
	ON_COMMAND( ID_SELECTION_COMBINE, &CMainFrame::OnSelectionCombine )
	ON_COMMAND( ID_PATCH_COMBINE, &CMainFrame::OnPatchCombine )
	ON_COMMAND( ID_SHOW_DOOM, &CMainFrame::OnShowDoom )
	ON_COMMAND( ID_VIEW_RENDERMODE, &CMainFrame::OnViewRendermode )
	ON_COMMAND( ID_VIEW_REBUILDRENDERDATA, &CMainFrame::OnViewRebuildrenderdata )
	ON_COMMAND( ID_VIEW_REALTIMEREBUILD, &CMainFrame::OnViewRealtimerebuild )
	ON_COMMAND( ID_VIEW_RENDERENTITYOUTLINES, &CMainFrame::OnViewRenderentityoutlines )
	ON_COMMAND( ID_VIEW_MATERIALANIMATION, &CMainFrame::OnViewMaterialanimation )
	ON_COMMAND( ID_SELECT_AXIALTEXTURE_BYWIDTH, &CMainFrame::OnAxialTextureByWidth )
	ON_COMMAND( ID_SELECT_AXIALTEXTURE_BYHEIGHT, &CMainFrame::OnAxialTextureByHeight )
	ON_COMMAND( ID_SELECT_AXIALTEXTURE_ARBITRARY, &CMainFrame::OnAxialTextureArbitrary )
	ON_COMMAND( ID_SELECTION_EXPORT_TOOBJ, &CMainFrame::OnSelectionExportToobj )
	ON_COMMAND( ID_SELECTION_EXPORT_TOCM, &CMainFrame::OnSelectionExportToCM )
	ON_COMMAND( ID_VIEW_RENDERSELECTION, &CMainFrame::OnViewRenderselection )
	ON_COMMAND( ID_SELECT_NOMODELS, &CMainFrame::OnSelectNomodels )
	ON_COMMAND( ID_VIEW_SHOW_SHOWVISPORTALS, &CMainFrame::OnViewShowShowvisportals )
	ON_COMMAND( ID_VIEW_SHOW_NODRAW, &CMainFrame::OnViewShowNoDraw )
	ON_COMMAND( ID_VIEW_RENDERSOUND, &CMainFrame::OnViewRendersound )
	ON_COMMAND( ID_SOUND_SHOWSOUNDVOLUMES, &CMainFrame::OnSoundShowsoundvolumes )
	ON_COMMAND( ID_SOUND_SHOWSELECTEDSOUNDVOLUMES, &CMainFrame::OnSoundShowselectedsoundvolumes )
	ON_COMMAND( ID_PATCH_NURBEDITOR, &CMainFrame::OnNurbEditor )
	ON_COMMAND( ID_SELECT_COMPLETE_ENTITY, &CMainFrame::OnSelectCompleteEntity )
	ON_COMMAND( ID_PRECISION_CURSOR_CYCLE , &CMainFrame::OnPrecisionCursorCycle )
	ON_COMMAND( ID_MATERIALS_GENERATEMATERIALSLIST, OnGenerateMaterialsList )
	ON_COMMAND( ID_SELECTION_VIEW_WIREFRAMEON, &CMainFrame::OnSelectionWireFrameOn )
	ON_COMMAND( ID_SELECTION_VIEW_WIREFRAMEOFF, &CMainFrame::OnSelectionWireFrameOff )
	ON_COMMAND( ID_SELECTION_VIEW_VISIBLEON, &CMainFrame::OnSelectionVisibleOn )
	ON_COMMAND( ID_SELECTION_VIEW_VISIBLEOFF, &CMainFrame::OnSelectionVisibleOff )
	//}}AFX_MSG_MAP
	ON_COMMAND_RANGE( IDMRU, IDMRU_END, &CMainFrame::OnMru )
	ON_COMMAND_RANGE( ID_VIEW_NEAREST, ID_TEXTURES_FLATSHADE, &CMainFrame::OnViewNearest )
	ON_COMMAND_RANGE( ID_GRID_POINT0625, ID_GRID_64, &CMainFrame::OnGrid1 )
	ON_COMMAND( ID_AUTOCAULK, &CMainFrame::OnAutocaulk )
	ON_UPDATE_COMMAND_UI( ID_AUTOCAULK, &CMainFrame::OnUpdateAutocaulk )
	ON_COMMAND( ID_SELECT_ALLTARGETS, &CMainFrame::OnSelectAlltargets )
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,	// status line indicator
	ID_SEPARATOR,	// status line indicator
	ID_SEPARATOR,	// status line indicator
	ID_SEPARATOR,	// status line indicator
	ID_SEPARATOR,	// status line indicator
	ID_SEPARATOR,	// status line indicator
};

std::chrono::steady_clock::time_point lastFrameTime;

idCVar radiant_cameraMoveSpeed( "radiant_cameraMoveSpeed", "200", CVAR_TOOL | CVAR_FLOAT, "how fast the camera is going to move in radiant." );

//
// =======================================================================================================================
//    CMainFrame construction/destruction
// =======================================================================================================================
//
CMainFrame::CMainFrame() noexcept
{
	m_bDoLoop = false;
	g_pParentWnd = this;
	m_pXYWnd = NULL;
	m_pCamWnd = NULL;
	m_pZWnd = NULL;
	m_pYZWnd = NULL;
	m_pXZWnd = NULL;
	m_pActiveXY = NULL;
	m_bCamPreview = true;
	nurbMode = 0;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
CMainFrame::~CMainFrame()
{
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void HandlePopup( CWnd* pWindow, unsigned int uId )
{
	// Get the current position of the mouse
	CPoint	ptMouse;
	GetCursorPos( &ptMouse );

	// Load up a menu that has the options we are looking for in it
	CMenu	mnuPopup;
	VERIFY( mnuPopup.LoadMenu( uId ) );
	mnuPopup.GetSubMenu( 0 )->TrackPopupMenu
	(
		TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
		ptMouse.x,
		ptMouse.y,
		pWindow
	);
	mnuPopup.DestroyMenu();

	// Set focus back to window
	pWindow->SetFocus();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::SetButtonMenuStates()
{
	CMenu*	pMenu = GetMenu();
	if( pMenu )
	{
		//
		pMenu->CheckMenuItem( ID_VIEW_SHOWNAMES, MF_BYCOMMAND | MF_CHECKED );
		pMenu->CheckMenuItem( ID_VIEW_SHOWCOORDINATES, MF_BYCOMMAND | MF_CHECKED );
		pMenu->CheckMenuItem( ID_VIEW_SHOWLIGHTS, MF_BYCOMMAND | MF_CHECKED );
		pMenu->CheckMenuItem( ID_VIEW_SHOWCOMBATNODES, MF_BYCOMMAND | MF_CHECKED );
		pMenu->CheckMenuItem( ID_VIEW_SHOWPATH, MF_BYCOMMAND | MF_CHECKED );
		pMenu->CheckMenuItem( ID_VIEW_SHOWWATER, MF_BYCOMMAND | MF_CHECKED );
		pMenu->CheckMenuItem( ID_VIEW_SHOWWORLD, MF_BYCOMMAND | MF_CHECKED );
		pMenu->CheckMenuItem( ID_VIEW_SHOWCLIP, MF_BYCOMMAND | MF_CHECKED );
		pMenu->CheckMenuItem( ID_VIEW_SHOWTRIGGERS, MF_BYCOMMAND | MF_CHECKED );
		pMenu->CheckMenuItem( ID_VIEW_SHOWHINT, MF_BYCOMMAND | MF_CHECKED );
		pMenu->CheckMenuItem( ID_VIEW_SHOWCAULK, MF_BYCOMMAND | MF_CHECKED );
		pMenu->CheckMenuItem( ID_VIEW_SHOW_SHOWVISPORTALS, MF_BYCOMMAND | MF_CHECKED );
		pMenu->CheckMenuItem( ID_VIEW_SHOW_NODRAW, MF_BYCOMMAND | MF_CHECKED );
		pMenu->CheckMenuItem( ID_VIEW_SHOWANGLES, MF_BYCOMMAND | MF_CHECKED );

		if( !g_qeglobals.d_savedinfo.show_names )
		{
			pMenu->CheckMenuItem( ID_VIEW_SHOWNAMES, MF_BYCOMMAND | MF_UNCHECKED );
		}

		if( !g_qeglobals.d_savedinfo.show_coordinates )
		{
			pMenu->CheckMenuItem( ID_VIEW_SHOWCOORDINATES, MF_BYCOMMAND | MF_UNCHECKED );
		}

		if( g_qeglobals.d_savedinfo.exclude & EXCLUDE_LIGHTS )
		{
			pMenu->CheckMenuItem( ID_VIEW_SHOWLIGHTS, MF_BYCOMMAND | MF_UNCHECKED );
		}

		if( g_qeglobals.d_savedinfo.exclude & EXCLUDE_COMBATNODES )
		{
			pMenu->CheckMenuItem( ID_VIEW_SHOWCOMBATNODES, MF_BYCOMMAND | MF_UNCHECKED );
		}

		if( g_qeglobals.d_savedinfo.exclude & EXCLUDE_ENT )
		{
			pMenu->CheckMenuItem( ID_VIEW_SHOWENT, MF_BYCOMMAND | MF_UNCHECKED );
		}

		if( g_qeglobals.d_savedinfo.exclude & EXCLUDE_PATHS )
		{
			pMenu->CheckMenuItem( ID_VIEW_SHOWPATH, MF_BYCOMMAND | MF_UNCHECKED );
		}

		if( g_qeglobals.d_savedinfo.exclude & EXCLUDE_DYNAMICS )
		{
			pMenu->CheckMenuItem( ID_VIEW_SHOWWATER, MF_BYCOMMAND | MF_UNCHECKED );
		}

		if( g_qeglobals.d_savedinfo.exclude & EXCLUDE_WORLD )
		{
			pMenu->CheckMenuItem( ID_VIEW_SHOWWORLD, MF_BYCOMMAND | MF_UNCHECKED );
		}

		if( g_qeglobals.d_savedinfo.exclude & EXCLUDE_CLIP )
		{
			pMenu->CheckMenuItem( ID_VIEW_SHOWCLIP, MF_BYCOMMAND | MF_UNCHECKED );
		}

		if( g_qeglobals.d_savedinfo.exclude & EXCLUDE_TRIGGERS )
		{
			pMenu->CheckMenuItem( ID_VIEW_SHOWTRIGGERS, MF_BYCOMMAND | MF_UNCHECKED );
		}

		if( g_qeglobals.d_savedinfo.exclude & EXCLUDE_HINT )
		{
			pMenu->CheckMenuItem( ID_VIEW_SHOWHINT, MF_BYCOMMAND | MF_UNCHECKED );
		}

		if( g_qeglobals.d_savedinfo.exclude & EXCLUDE_CAULK )
		{
			pMenu->CheckMenuItem( ID_VIEW_SHOWCAULK, MF_BYCOMMAND | MF_UNCHECKED );
		}

		if( g_qeglobals.d_savedinfo.exclude & EXCLUDE_VISPORTALS )
		{
			pMenu->CheckMenuItem( ID_VIEW_SHOW_SHOWVISPORTALS, MF_BYCOMMAND | MF_UNCHECKED );
		}

		if( g_qeglobals.d_savedinfo.exclude & EXCLUDE_NODRAW )
		{
			pMenu->CheckMenuItem( ID_VIEW_SHOW_NODRAW, MF_BYCOMMAND | MF_UNCHECKED );
		}

		if( g_qeglobals.d_savedinfo.exclude & EXCLUDE_ANGLES )
		{
			pMenu->CheckMenuItem( ID_VIEW_SHOWANGLES, MF_BYCOMMAND | MF_UNCHECKED );
		}

		pMenu->CheckMenuItem( ID_TOGGLE_LOCK, MF_BYCOMMAND | ( g_PrefsDlg.m_bTextureLock ) ? MF_CHECKED : MF_UNCHECKED );
		pMenu->CheckMenuItem
		(
			ID_TOGGLE_ROTATELOCK,
			MF_BYCOMMAND | ( g_PrefsDlg.m_bRotateLock ) ? MF_CHECKED : MF_UNCHECKED
		);
		pMenu->CheckMenuItem
		(
			ID_VIEW_CUBICCLIPPING,
			MF_BYCOMMAND | ( g_PrefsDlg.m_bCubicClipping ) ? MF_CHECKED : MF_UNCHECKED
		);
		pMenu->CheckMenuItem
		(
			ID_VIEW_OPENGLLIGHTING,
			MF_BYCOMMAND | ( g_PrefsDlg.m_bGLLighting ) ? MF_CHECKED : MF_UNCHECKED
		);
		pMenu->CheckMenuItem( ID_SNAPTOGRID, MF_BYCOMMAND | ( !g_PrefsDlg.m_bNoClamp ) ? MF_CHECKED : MF_UNCHECKED );

		SetToolbarButtonCheckState( m_wndToolBar, ID_VIEW_CUBICCLIPPING, ( g_PrefsDlg.m_bCubicClipping ) ? TRUE : FALSE );

		int n = g_PrefsDlg.m_nTextureScale;
		int id;
		switch( n )
		{
			case 10:
				id = ID_TEXTURES_TEXTUREWINDOWSCALE_10;
				break;
			case 25:
				id = ID_TEXTURES_TEXTUREWINDOWSCALE_25;
				break;
			case 50:
				id = ID_TEXTURES_TEXTUREWINDOWSCALE_50;
				break;
			case 200:
				id = ID_TEXTURES_TEXTUREWINDOWSCALE_200;
				break;
			default:
				id = ID_TEXTURES_TEXTUREWINDOWSCALE_100;
				break;
		}

		CheckTextureScale( id );
	}

	LoadMruInReg( g_qeglobals.d_lpMruMenu, "Software\\" EDITOR_REGISTRY_KEY "\\MRU" );
	PlaceMenuMRUItem( g_qeglobals.d_lpMruMenu, ::GetSubMenu( ::GetMenu( GetSafeHwnd() ), 0 ), ID_FILE_EXIT );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::ShowMenuItemKeyBindings( CMenu* pMenu )
{
	int				i, j;
	char			key[1024], *ptr;
	MENUITEMINFO	MenuItemInfo;

	// return;
	for( i = 0; i < g_nCommandCount; i++ )
	{
		memset( &MenuItemInfo, 0, sizeof( MENUITEMINFO ) );
		MenuItemInfo.cbSize = sizeof( MENUITEMINFO );
		MenuItemInfo.fMask = MIIM_TYPE;
		MenuItemInfo.dwTypeData = key;
		MenuItemInfo.cch = sizeof( key );
		if( !pMenu->GetMenuItemInfo( g_Commands[i].m_nCommand, &MenuItemInfo ) )
		{
			continue;
		}

		if( MenuItemInfo.fType != MFT_STRING )
		{
			continue;
		}

		ptr = strchr( key, '\t' );
		if( ptr )
		{
			*ptr = '\0';
		}

		strcat( key, "\t" );
		if( g_Commands[i].m_nModifiers )  	// are there modifiers present?
		{
			if( g_Commands[i].m_nModifiers & RAD_SHIFT )
			{
				strcat( key, "Shift-" );
			}

			if( g_Commands[i].m_nModifiers & RAD_ALT )
			{
				strcat( key, "Alt-" );
			}

			if( g_Commands[i].m_nModifiers & RAD_CONTROL )
			{
				strcat( key, "Ctrl-" );
			}
		}

		for( j = 0; j < g_nKeyCount; j++ )
		{
			if( g_Commands[i].m_nKey == g_Keys[j].m_nVKKey )
			{
				strcat( key, g_Keys[j].m_strName );
				break;
			}
		}

		if( j >= g_nKeyCount )
		{
			sprintf( &key[strlen( key )], "%c", g_Commands[i].m_nKey );
		}

		memset( &MenuItemInfo, 0, sizeof( MENUITEMINFO ) );
		MenuItemInfo.cbSize = sizeof( MENUITEMINFO );
		MenuItemInfo.fMask = MIIM_TYPE;
		MenuItemInfo.fType = MFT_STRING;
		MenuItemInfo.dwTypeData = key;
		MenuItemInfo.cch = strlen( key );
		SetMenuItemInfo( pMenu->m_hMenu, g_Commands[i].m_nCommand, FALSE, &MenuItemInfo );
	}
}

/*
==============
MFCCreate
==============
*/
void MFCCreate( HINSTANCE hInstance )
{
	int i = sizeof( g_qeglobals.d_savedinfo );
	long l = i;

	g_qeglobals.d_savedinfo.exclude |= ( EXCLUDE_HINT | EXCLUDE_CLIP );
	LoadRegistryInfo( "radiant_SavedInfo", &g_qeglobals.d_savedinfo, &l );

	int nOldSize = g_qeglobals.d_savedinfo.iSize;
	if( g_qeglobals.d_savedinfo.iSize != sizeof( g_qeglobals.d_savedinfo ) )
	{
		// fill in new defaults
		g_qeglobals.d_savedinfo.iSize = sizeof( g_qeglobals.d_savedinfo );
		g_qeglobals.d_savedinfo.iTexMenu = ID_VIEW_BILINEARMIPMAP;
		g_qeglobals.d_savedinfo.m_nTextureTweak = 1.0;

		//g_qeglobals.d_savedinfo.exclude = INCLUDE_EASY | INCLUDE_NORMAL | INCLUDE_HARD | INCLUDE_DEATHMATCH;
		g_qeglobals.d_savedinfo.show_coordinates = true;
		g_qeglobals.d_savedinfo.show_names       = false;

		for( i = 0 ; i < 3 ; i++ )
		{
			g_qeglobals.d_savedinfo.colors[COLOR_TEXTUREBACK][i] = 0;
			g_qeglobals.d_savedinfo.colors[COLOR_GRIDBACK][i] = 1.0;
			g_qeglobals.d_savedinfo.colors[COLOR_GRIDMINOR][i] = 0.75;
			g_qeglobals.d_savedinfo.colors[COLOR_GRIDMAJOR][i] = 0.5;
			g_qeglobals.d_savedinfo.colors[COLOR_CAMERABACK][i] = 0.25;
		}

		g_qeglobals.d_savedinfo.colors[COLOR_GRIDBLOCK][0] = 0.0;
		g_qeglobals.d_savedinfo.colors[COLOR_GRIDBLOCK][1] = 0.0;
		g_qeglobals.d_savedinfo.colors[COLOR_GRIDBLOCK][2] = 1.0;

		g_qeglobals.d_savedinfo.colors[COLOR_GRIDTEXT][0] = 0.0;
		g_qeglobals.d_savedinfo.colors[COLOR_GRIDTEXT][1] = 0.0;
		g_qeglobals.d_savedinfo.colors[COLOR_GRIDTEXT][2] = 0.0;

		g_qeglobals.d_savedinfo.colors[COLOR_SELBRUSHES][0] = 1.0;
		g_qeglobals.d_savedinfo.colors[COLOR_SELBRUSHES][1] = 0.0;
		g_qeglobals.d_savedinfo.colors[COLOR_SELBRUSHES][2] = 0.0;

		g_qeglobals.d_savedinfo.colors[COLOR_CLIPPER][0] = 0.0;
		g_qeglobals.d_savedinfo.colors[COLOR_CLIPPER][1] = 0.0;
		g_qeglobals.d_savedinfo.colors[COLOR_CLIPPER][2] = 1.0;

		g_qeglobals.d_savedinfo.colors[COLOR_BRUSHES][0] = 0.0;
		g_qeglobals.d_savedinfo.colors[COLOR_BRUSHES][1] = 0.0;
		g_qeglobals.d_savedinfo.colors[COLOR_BRUSHES][2] = 0.0;

		g_qeglobals.d_savedinfo.colors[COLOR_VIEWNAME][0] = 0.5;
		g_qeglobals.d_savedinfo.colors[COLOR_VIEWNAME][1] = 0.0;
		g_qeglobals.d_savedinfo.colors[COLOR_VIEWNAME][2] = 0.75;


		// old size was smaller, reload original prefs
		if( nOldSize > 0 && nOldSize < sizeof( g_qeglobals.d_savedinfo ) )
		{
			long lOldSize = nOldSize;
			LoadRegistryInfo( "radiant_SavedInfo", &g_qeglobals.d_savedinfo, &lOldSize );
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
int CMainFrame::OnCreate( LPCREATESTRUCT lpCreateStruct )
{
	char*	pBuffer = g_strAppPath.GetBufferSetLength( _MAX_PATH + 1 );
	int		nResult = ::GetModuleFileName( NULL, pBuffer, _MAX_PATH );
	ASSERT( nResult != 0 );
	pBuffer[g_strAppPath.ReverseFind( '\\' ) + 1] = '\0';
	g_strAppPath.ReleaseBuffer();

	com_editors |= EDITOR_RADIANT;

	InitCommonControls();
	g_qeglobals.d_hInstance = AfxGetInstanceHandle();
	MFCCreate( AfxGetInstanceHandle() );

	// g_PrefsDlg.LoadPrefs();
	if( CFrameWndEx::OnCreate( lpCreateStruct ) == -1 )
	{
		return -1;
	}

	// Load the icon
	HICON hIcon = AfxGetApp()->LoadIconA( IDI_MAINFRAME_EDITOR );
	if( hIcon )
	{
		SetIcon( hIcon, TRUE );
		SetIcon( hIcon, FALSE );
	}

	SetWindowTheme( GetSafeHwnd(), L"EXPLORER", NULL );

	if( !m_wndMenuBar.Create( this ) )
	{
		TRACE0( "Failed to create menubar\n" );
		return -1;      // fail to create
	}

	m_wndMenuBar.SetPaneStyle( m_wndMenuBar.GetPaneStyle() | CBRS_SIZE_DYNAMIC | CBRS_TOOLTIPS | CBRS_FLYBY );

	// prevent the menu bar from taking the focus on activation
	CMFCPopupMenu::SetForceMenuFocus( FALSE );

	if( !m_wndToolBar.CreateEx( this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC ) ||
			!m_wndToolBar.LoadToolBar( IDR_TOOLBAR ) )
	{
		TRACE0( "Failed to create toolbar\n" );
		return -1;      // fail to create
	}

	/*
	CString strToolBarName;
	bNameValid = strToolBarName.LoadString(IDS_TOOLBAR_STANDARD);
	ASSERT(bNameValid);
	m_wndToolBar.SetWindowText(strToolBarName);

	CString strCustomize;
	bNameValid = strCustomize.LoadString(IDS_TOOLBAR_CUSTOMIZE);
	ASSERT(bNameValid);
	m_wndToolBar.EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, strCustomize);
	*/

	if( !m_wndStatusBar.Create( this ) )
	{
		TRACE0( "Failed to create status bar\n" );
		return -1;      // fail to create
	}
	m_wndStatusBar.SetIndicators( indicators, sizeof( indicators ) / sizeof( UINT ) );

	m_bCamPreview = true;

	SetToolbarButtonCheckState( m_wndToolBar, ID_SCALELOCKX, FALSE );
	SetToolbarButtonCheckState( m_wndToolBar, ID_SCALELOCKY, FALSE );
	SetToolbarButtonCheckState( m_wndToolBar, ID_SCALELOCKZ, FALSE );
	SetToolbarButtonCheckState( m_wndToolBar, ID_SELECT_BYBOUNDINGBRUSH, FALSE );
	SetToolbarButtonCheckState( m_wndToolBar, ID_SELECT_BRUSHESONLY, FALSE );
	SetToolbarButtonCheckState( m_wndToolBar, ID_PATCH_SHOWBOUNDINGBOX, FALSE );
	SetToolbarButtonCheckState( m_wndToolBar, ID_PATCH_WELD, TRUE );
	SetToolbarButtonCheckState( m_wndToolBar, ID_PATCH_DRILLDOWN, TRUE );
	SetToolbarButtonCheckState( m_wndToolBar, ID_SHOW_LIGHTVOLUMES, FALSE );
	SetToolbarButtonCheckState( m_wndToolBar, ID_SHOW_LIGHTTEXTURES, FALSE );
	SetToolbarButtonCheckState( m_wndToolBar, ID_SELECTION_MOVEONLY, FALSE );
	SetToolbarButtonCheckState( m_wndToolBar, ID_SOUND_SHOWSOUNDVOLUMES, g_qeglobals.d_savedinfo.showSoundAlways );
	SetToolbarButtonCheckState( m_wndToolBar, ID_SOUND_SHOWSELECTEDSOUNDVOLUMES, g_qeglobals.d_savedinfo.showSoundWhenSelected );

	m_wndMenuBar.EnableDocking( CBRS_ALIGN_ANY );
	m_wndToolBar.EnableDocking( CBRS_ALIGN_ANY );
	EnableDocking( CBRS_ALIGN_ANY );
	DockPane( &m_wndMenuBar );
	DockPane( &m_wndToolBar );

	g_nScaleHow = 0;

	g_qeglobals.d_lpMruMenu = CreateMruMenuDefault();

	m_bAutoMenuEnable = FALSE;

	LoadCommandMap();

	//CMenu *pMenu = GetMenu();
	//ShowMenuItemKeyBindings(pMenu);

	CFont*	pFont = new CFont();
	pFont->CreatePointFont( g_PrefsDlg.m_nStatusSize * 10, "Segoe UI" );
	m_wndStatusBar.SetFont( pFont );


	if( g_PrefsDlg.m_bRunBefore == FALSE )
	{
		g_PrefsDlg.m_bRunBefore = TRUE;
		g_PrefsDlg.SavePrefs();

		/*
		 * if (MessageBox("Would you like QERadiant to build and load a default project?
		 * If this is the first time you have run QERadiant or you are not familiar with
		 * editing QE4 project files directly, this is HIGHLY recommended", "Create a
		 * default project?", MB_YESNO) == IDYES) { OnFileNewproject(); }
		 */
	}
	else
	{
		// load plugins before the first Map_LoadFile required for model plugins
		if( g_PrefsDlg.m_bLoadLastMap && g_PrefsDlg.m_strLastMap.GetLength() > 0 )
		{
			Map_LoadFile( g_PrefsDlg.m_strLastMap.GetBuffer( 0 ) );
		}
	}

	SetGridStatus();
	SetTexValStatus();
	SetButtonMenuStates();
	LoadBarState( "RadiantToolBars2" );

	SetActiveXY( m_pXYWnd );
	m_pXYWnd->SetFocus();

	// set the visual manager used to draw all user interface elements
	CMFCVisualManager::SetDefaultManager( RUNTIME_CLASS( CMFCVisualManagerVS2008 ) );

	// enable Visual Studio 2005 style docking window behavior
	CDockingManager::SetDockingMode( DT_SMART );

	// enable Visual Studio 2005 style docking window auto-hide behavior
	EnableAutoHidePanes( CBRS_ALIGN_ANY );

	PostMessage( WM_KEYDOWN, 'O', NULL );

	if( radiant_entityMode.GetBool() )
	{
		g_qeglobals.d_savedinfo.exclude |= ( EXCLUDE_PATHS | EXCLUDE_CLIP | EXCLUDE_CAULK | EXCLUDE_VISPORTALS | EXCLUDE_NODRAW | EXCLUDE_TRIGGERS );
	}

	// Enable toolbar and docking window menu replacement
	//EnablePaneMenu(TRUE, ID_VIEW_CUSTOMIZE, strCustomize, ID_VIEW_TOOLBAR);

	// enable quick (Alt+drag) toolbar customization
	CMFCToolBar::EnableQuickCustomization();

	Sys_UpdateWindows( W_ALL );
	return 0;
}

/*
 =======================================================================================================================
 =======================================================================================================================
*/
BOOL CMainFrame::PreCreateWindow( CREATESTRUCT& cs )
{
	if( !CFrameWndEx::PreCreateWindow( cs ) )
	{
		return FALSE;
	}
	return TRUE;
}

/*
 =======================================================================================================================
 =======================================================================================================================
*/

void FindReplace( CString& strContents, const char* pTag, const char* pValue )
{
	if( strcmp( pTag, pValue ) == 0 )
	{
		return;
	}
	for( int nPos = strContents.Find( pTag ); nPos >= 0; nPos = strContents.Find( pTag ) )
	{
		int nRightLen = strContents.GetLength() - strlen( pTag ) - nPos;
		CString strLeft = strContents.Left( nPos );
		CString strRight = strContents.Right( nRightLen );
		strLeft += pValue;
		strLeft += strRight;
		strContents = strLeft;
	}
}

void CMainFrame::LoadCommandMap()
{
	CString strINI;
	char	pBuff[1024];
	strINI = g_strAppPath;
	strINI += "\\radiant.ini";

	for( int i = 0; i < g_nCommandCount; i++ )
	{
		int nLen = GetPrivateProfileString( "Commands", g_Commands[i].m_strCommand, "", pBuff, 1024, strINI );
		if( nLen > 0 )
		{
			CString strBuff = pBuff;
			strBuff.TrimLeft();
			strBuff.TrimRight();

			int nSpecial = strBuff.Find( "+alt" );
			g_Commands[i].m_nModifiers = 0;
			if( nSpecial >= 0 )
			{
				g_Commands[i].m_nModifiers |= RAD_ALT;
				FindReplace( strBuff, "+alt", "" );
			}

			nSpecial = strBuff.Find( "+ctrl" );
			if( nSpecial >= 0 )
			{
				g_Commands[i].m_nModifiers |= RAD_CONTROL;
				FindReplace( strBuff, "+ctrl", "" );
			}

			nSpecial = strBuff.Find( "+shift" );
			if( nSpecial >= 0 )
			{
				g_Commands[i].m_nModifiers |= RAD_SHIFT;
				FindReplace( strBuff, "+shift", "" );
			}

			strBuff.TrimLeft();
			strBuff.TrimRight();
			strBuff.MakeUpper();
			if( nLen == 1 )  	// most often case.. deal with first
			{
				g_Commands[i].m_nKey = __toascii( strBuff.GetAt( 0 ) );
			}
			else  				// special key
			{
				for( int j = 0; j < g_nKeyCount; j++ )
				{
					if( strBuff.CompareNoCase( g_Keys[j].m_strName ) == 0 )
					{
						g_Commands[i].m_nKey = g_Keys[j].m_nVKKey;
						break;
					}
				}
			}
		}
	}
}

// CMainFrame diagnostics
#ifdef _DEBUG

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::AssertValid() const
{
	CFrameWndEx::AssertValid();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::Dump( CDumpContext& dc ) const
{
	CFrameWndEx::Dump( dc );
}
#endif // _DEBUG

//
// =======================================================================================================================
//    CMainFrame message handlers
// =======================================================================================================================
//
void CMainFrame::CreateQEChildren()
{
	QE_Init();

	common->Printf( "Entering message loop\n" );

	m_bDoLoop = true;
	SetTimer( QE_TIMER0, 100, NULL );
	SetTimer( QE_TIMER1, g_PrefsDlg.m_nAutoSave * 60 * 1000, NULL );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::RoutineProcessing()
{
	if( m_bDoLoop )
	{
		double	time = 0.0;
		static double oldtime = 0.0;
		double	delta = 0.0;

		time = Sys_DoubleTime();
		delta = time - oldtime;
		oldtime = time;
		if( delta > 0.2 )
		{
			delta = 0.2;
		}

		// run time dependant behavior
		if( m_pCamWnd )
		{
			m_pCamWnd->Cam_MouseControl( delta );
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
bool MouseDown()
{
	if( ::GetAsyncKeyState( VK_LBUTTON ) )
	{
		return true;
	}

	if( ::GetAsyncKeyState( VK_RBUTTON ) )
	{
		return true;
	}

	if( ::GetAsyncKeyState( VK_MBUTTON ) )
	{
		return true;
	}

	return false;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */

void CMainFrame::OnTimer( UINT_PTR nIDEvent )
{
	static bool autoSavePending = false;

	if( nIDEvent == QE_TIMER0 && !MouseDown() )
	{
		QE_CountBrushesAndUpdateStatusBar();
	}
	if( nIDEvent == QE_TIMER1 || autoSavePending )
	{
		if( MouseDown() )
		{
			autoSavePending = true;
			return;
		}
		if( Sys_Waiting() )
		{
			autoSavePending = true;
			return;
		}
		QE_CheckAutoSave();
		autoSavePending = false;
	}
}

struct SplitInfo
{
	int m_nMin;
	int m_nCur;
};

/*
 =======================================================================================================================
 =======================================================================================================================
 */
bool LoadWindowPlacement( HWND hwnd, const char* pName )
{
	WINDOWPLACEMENT wp;
	wp.length = sizeof( WINDOWPLACEMENT );

	LONG lSize = sizeof( wp );
	if( LoadRegistryInfo( pName, &wp, &lSize ) )
	{
		::SetWindowPlacement( hwnd, &wp );
		return true;
	}

	return false;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void SaveWindowPlacement( HWND hwnd, const char* pName )
{
	WINDOWPLACEMENT wp;
	wp.length = sizeof( WINDOWPLACEMENT );
	if( ::GetWindowPlacement( hwnd, &wp ) )
	{
		SaveRegistryInfo( pName, &wp, sizeof( wp ) );
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void SaveDialogPlacement( CDialog* dlg, const char* pName )
{
	WINDOWPLACEMENT wp;
	wp.length = sizeof( WINDOWPLACEMENT );
	if( dlg->GetWindowPlacement( &wp ) )
	{
		SaveRegistryInfo( pName, &wp, sizeof( wp ) );
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnDestroy()
{
	KillTimer( QE_TIMER0 );

	SaveBarState( "RadiantToolBars2" );

	// FIXME original mru stuff needs replaced with mfc stuff
	SaveMruInReg( g_qeglobals.d_lpMruMenu, "Software\\" EDITOR_REGISTRY_KEY "\\MRU" );

	DeleteMruMenu( g_qeglobals.d_lpMruMenu );

	SaveRegistryInfo( "radiant_SavedInfo", &g_qeglobals.d_savedinfo, sizeof( g_qeglobals.d_savedinfo ) );

	SaveWindowPlacement( GetSafeHwnd(), "radiant_MainWindowPlace" );
	SaveWindowState( g_Inspectors->texWnd.GetSafeHwnd(), "radiant_texwindow" );

	if( m_pXYWnd->GetSafeHwnd() )
	{
		m_pXYWnd->SendMessage( WM_DESTROY, 0, 0 );
	}

	delete m_pXYWnd;
	m_pXYWnd = NULL;

	if( m_pYZWnd->GetSafeHwnd() )
	{
		m_pYZWnd->SendMessage( WM_DESTROY, 0, 0 );
	}

	delete m_pYZWnd;
	m_pYZWnd = NULL;

	if( m_pXZWnd->GetSafeHwnd() )
	{
		m_pXZWnd->SendMessage( WM_DESTROY, 0, 0 );
	}

	delete m_pXZWnd;
	m_pXZWnd = NULL;

	if( m_pZWnd->GetSafeHwnd() )
	{
		m_pZWnd->SendMessage( WM_DESTROY, 0, 0 );
	}

	delete m_pZWnd;
	m_pZWnd = NULL;

	if( m_pCamWnd->GetSafeHwnd() )
	{
		m_pCamWnd->SendMessage( WM_DESTROY, 0, 0 );
	}

	delete m_pCamWnd;
	m_pCamWnd = NULL;

	if( idStr::Icmp( currentmap, "unnamed.map" ) != 0 )
	{
		g_PrefsDlg.m_strLastMap = currentmap;
		g_PrefsDlg.SavePrefs();
	}

	CleanUpEntities();

	while( active_brushes.next != &active_brushes )
	{
		Brush_Free( active_brushes.next, false );
	}

	while( selected_brushes.next != &selected_brushes )
	{
		Brush_Free( selected_brushes.next, false );
	}

	while( filtered_brushes.next != &filtered_brushes )
	{
		Brush_Free( filtered_brushes.next, false );
	}

	while( entities.next != &entities )
	{
		delete entities.next;
	}

	if( world_entity )
	{
		delete world_entity;
	}

	//
	// FIXME: idMaterial
	// if (notexture) { // Timo // Surface properties plugin #ifdef _DEBUG if (
	// !notexture->pData ) common->Printf("WARNING: found a qtexture_t* with no
	// IPluginQTexture\n"); #endif if ( notexture->pData )
	// GETPLUGINTEXDEF(notexture)->DecRef(); Mem_Free(notexture); }
	// if (current_texture) free(current_texture);
	//

	// FIXME: idMaterial FreeShaders();
	CFrameWndEx::OnDestroy();

	AfxGetApp()->ExitInstance();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnClose()
{
	if( ConfirmModified() )
	{
		g_Inspectors->SaveWindowPlacement();
		CFrameWndEx::OnClose();
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnKeyUp( UINT nChar, UINT nRepCnt, UINT nFlags )
{
	// run through our list to see if we have a handler for nChar

	for( int i = 0; i < g_nCommandCount; i++ )
	{
		if( g_Commands[i].m_nKey == nChar )  	// find a match?
		{
			bool	bGo = true;
			if( g_Commands[i].m_nModifiers & RAD_PRESS )
			{
				int nModifiers = g_Commands[i].m_nModifiers & ~RAD_PRESS;
				if( nModifiers )  				// are there modifiers present?
				{
					if( nModifiers & RAD_ALT )
					{
						if( !( GetAsyncKeyState( VK_MENU ) & 0x8000 ) )
						{
							bGo = false;
						}
					}

					if( nModifiers & RAD_CONTROL )
					{
						if( !( GetAsyncKeyState( VK_CONTROL ) & 0x8000 ) )
						{
							bGo = false;
						}
					}

					if( nModifiers & RAD_SHIFT )
					{
						if( !( GetAsyncKeyState( VK_SHIFT ) & 0x8000 ) )
						{
							bGo = false;
						}
					}
				}
				else  	// no modifiers make sure none of those keys are pressed
				{
					if( GetAsyncKeyState( VK_MENU ) & 0x8000 )
					{
						bGo = false;
					}

					if( GetAsyncKeyState( VK_CONTROL ) & 0x8000 )
					{
						bGo = false;
					}

					if( GetAsyncKeyState( VK_SHIFT ) & 0x8000 )
					{
						bGo = false;
					}
				}

				if( bGo )
				{
					SendMessage( WM_COMMAND, g_Commands[i].m_nCommand, 0 );
					break;
				}
			}
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
bool CamOK( unsigned int nKey )
{
	if( nKey == VK_UP || nKey == VK_LEFT || nKey == VK_RIGHT || nKey == VK_DOWN )
	{
		if( ::GetAsyncKeyState( nKey ) )
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	return true;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnSysKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags )
{
	// OnKeyDown(nChar, nRepCnt, nFlags);
	if( nChar == VK_DOWN )
	{
		OnKeyDown( nChar, nRepCnt, nFlags );
	}

	CFrameWndEx::OnSysKeyDown( nChar, nRepCnt, nFlags );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags )
{

	for( int i = 0; i < g_nCommandCount; i++ )
	{
		if( g_Commands[i].m_nKey == nChar )  	// find a match?
		{
			// check modifiers
			unsigned int	nState = 0;
			if( GetAsyncKeyState( VK_MENU ) & 0x8000 )
			{
				nState |= RAD_ALT;
			}

			if( GetAsyncKeyState( VK_CONTROL ) & 0x8000 )
			{
				nState |= RAD_CONTROL;
			}

			if( GetAsyncKeyState( VK_SHIFT ) & 0x8000 )
			{
				nState |= RAD_SHIFT;
			}

			if( ( g_Commands[i].m_nModifiers & 0x7 ) == nState )
			{
				SendMessage( WM_COMMAND, g_Commands[i].m_nCommand, 0 );
				break;
			}
		}
	}

	CFrameWndEx::OnKeyDown( nChar, nRepCnt, nFlags );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
BOOL CMainFrame::OnCreateClient( LPCREATESTRUCT lpcs, CCreateContext* pContext )
{

	g_Inspectors = new CInspectorDialog( this );
	g_Inspectors->Create( IDD_DIALOG_INSPECTORS, this );

	LoadWindowPlacement( g_Inspectors->GetSafeHwnd(), "radiant_InspectorsWindow" );
	g_Inspectors->ShowWindow( SW_SHOW );

	CRect r;
	g_Inspectors->GetWindowRect( r );

	//stupid hack to get the window resize itself properly
	r.DeflateRect( 0, 0, 0, 1 );
	g_Inspectors->MoveWindow( r );
	r.InflateRect( 0, 0, 0, 1 );
	g_Inspectors->MoveWindow( r );


	if( !LoadWindowPlacement( GetSafeHwnd(), "radiant_MainWindowPlace" ) )
	{
	}

	CRect rect( 5, 25, 100, 100 );
	CRect rctParent;
	GetClientRect( rctParent );

	m_pCamWnd = new CCamWnd();
	m_pCamWnd->Create( IDD_DIALOG_CAMERA, this );

	m_pZWnd = new CZWnd();
	m_pZWnd->Create( IDD_DIALOG_Z, this );

	m_pXYWnd = new CXYWnd();
	m_pXYWnd->Create( IDD_DIALOG_XY, this ); // XY_WINDOW_CLASS, "", QE3_CHILDSTYLE, rect, this, 1235);
	m_pXYWnd->SetViewType( ViewType::XY );

	m_pXZWnd = new CXYWnd();
	m_pXZWnd->Create( IDD_DIALOG_XZ, this );
	m_pXZWnd->SetViewType( ViewType::XZ );

	m_pYZWnd = new CXYWnd();
	m_pYZWnd->Create( IDD_DIALOG_YZ, this );
	m_pYZWnd->SetViewType( ViewType::YZ );

	m_pCamWnd->SetXYFriend( m_pXYWnd );

	CRect	rctWork;

	LoadWindowPlacement( m_pXYWnd->GetSafeHwnd(), "radiant_xywindow" );
	LoadWindowPlacement( m_pXZWnd->GetSafeHwnd(), "radiant_xzwindow" );
	LoadWindowPlacement( m_pYZWnd->GetSafeHwnd(), "radiant_yzwindow" );
	LoadWindowPlacement( m_pCamWnd->GetSafeHwnd(), "radiant_camerawindow" );
	LoadWindowPlacement( m_pZWnd->GetSafeHwnd(), "radiant_zwindow" );

	if( !g_PrefsDlg.m_bXZVis )
	{
		m_pXZWnd->ShowWindow( SW_HIDE );
	}

	if( !g_PrefsDlg.m_bYZVis )
	{
		m_pYZWnd->ShowWindow( SW_HIDE );
	}

	if( !g_PrefsDlg.m_bZVis )
	{
		m_pZWnd->ShowWindow( SW_HIDE );
	}

	CreateQEChildren();

	if( m_pXYWnd )
	{
		m_pXYWnd->SetActive( true );
	}

	Texture_SetMode( g_qeglobals.d_savedinfo.iTexMenu );

	g_Inspectors->SetMode( W_CONSOLE );
	return CFrameWndEx::OnCreateClient( lpcs, pContext );
}

CRect	g_rctOld( 0, 0, 0, 0 );

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnSize( UINT nType, int cx, int cy )
{
	CFrameWndEx::OnSize( nType, cx, cy );

	CRect	rctParent;
	GetClientRect( rctParent );
	float scaling_factor = Win_GetWindowScalingFactor( GetSafeHwnd() );

	UINT	nID;
	UINT	nStyle;
	int		nWidth;
	if( m_wndStatusBar.GetSafeHwnd() )
	{
		m_wndStatusBar.GetPaneInfo( 0, nID, nStyle, nWidth );
		m_wndStatusBar.SetPaneInfo( 0, nID, nStyle, rctParent.Width() * 0.15f * scaling_factor );
		m_wndStatusBar.GetPaneInfo( 1, nID, nStyle, nWidth );
		m_wndStatusBar.SetPaneInfo( 1, nID, nStyle, rctParent.Width() * 0.15f * scaling_factor );
		m_wndStatusBar.GetPaneInfo( 2, nID, nStyle, nWidth );
		m_wndStatusBar.SetPaneInfo( 2, nID, nStyle, rctParent.Width() * 0.15f * scaling_factor );
		m_wndStatusBar.GetPaneInfo( 3, nID, nStyle, nWidth );
		m_wndStatusBar.SetPaneInfo( 3, nID, nStyle, rctParent.Width() * 0.39f * scaling_factor );
		m_wndStatusBar.GetPaneInfo( 4, nID, nStyle, nWidth );
		m_wndStatusBar.SetPaneInfo( 4, nID, nStyle, rctParent.Width() * 0.15f * scaling_factor );
		m_wndStatusBar.GetPaneInfo( 5, nID, nStyle, nWidth );
		m_wndStatusBar.SetPaneInfo( 5, nID, nStyle, rctParent.Width() * 0.01f * scaling_factor );
	}
}

void	OpenDialog();
void	SaveAsDialog( bool bRegion );
void	Select_Ungroup();

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::ToggleCamera()
{
	if( m_bCamPreview )
	{
		m_bCamPreview = false;
	}
	else
	{
		m_bCamPreview = true;
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnFileExit()
{
	PostMessage( WM_CLOSE, 0, 0L );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnFileNew()
{
	if( ConfirmModified() )
	{
		Map_New();
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnFileOpen()
{
	if( ConfirmModified() )
	{
		OpenDialog();
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnFilePointfile()
{
	Pointfile_Clear();
	Pointfile_Check();
	Sys_UpdateWindows( W_ALL );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnFileSave()
{
	if( !strcmp( currentmap, "unnamed.map" ) )
	{
		SaveAsDialog( false );
	}
	else
	{
		Map_SaveFile( currentmap, false );
	}

	// DHM - _D3XP
	SetTimer( QE_TIMER1, g_PrefsDlg.m_nAutoSave * 60 * 1000, NULL );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnFileSaveas()
{
	SaveAsDialog( false );
}

/*
=======================================================================================================================
=======================================================================================================================
*/
static void AddSlash( CString& strPath )
{
	if( strPath.GetLength() > 0 )
	{
		if( strPath.GetAt( strPath.GetLength() - 1 ) != '\\' )
		{
			strPath += '\\';
		}
	}
}


/*
==================================================================================================
*/
void CMainFrame::OnViewShowModels()
{
	g_qeglobals.d_savedinfo.exclude ^= EXCLUDE_MODELS;

	Sys_UpdateWindows( W_ALL );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnView100()
{
	if( m_pXYWnd )
	{
		m_pXYWnd->SetScale( 1 );
	}

	if( m_pXZWnd )
	{
		m_pXZWnd->SetScale( 1 );
	}

	if( m_pYZWnd )
	{
		m_pYZWnd->SetScale( 1 );
	}

	Sys_UpdateWindows( W_XY | W_XY_OVERLAY );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnViewCenter()
{
	m_pCamWnd->Camera().angles[ROLL] = m_pCamWnd->Camera().angles[PITCH] = 0;
	m_pCamWnd->Camera().angles[YAW] = 22.5 * floor( ( m_pCamWnd->Camera().angles[YAW] + 11 ) / 22.5 );
	Sys_UpdateWindows( W_CAMERA | W_XY_OVERLAY );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnViewDownfloor()
{
	m_pCamWnd->Cam_ChangeFloor( false );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnViewInspector()
{
	if( g_Inspectors && g_Inspectors->GetSafeHwnd() )
	{
		if( g_Inspectors->IsWindowVisible() )
		{
			g_Inspectors->ShowWindow( SW_HIDE );
		}
		else
		{
			g_Inspectors->ShowWindow( SW_SHOW );
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnInspectorConsole()
{
	g_Inspectors->SetMode( W_CONSOLE );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnInspectorEntity()
{
	g_Inspectors->SetMode( W_ENTITY );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnInspectorMediaBrowser()
{
	g_Inspectors->SetMode( W_MEDIA );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnInspectorTexture()
{
	g_Inspectors->SetMode( W_TEXTURE );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnViewFront()
{
	m_pXYWnd->SetViewType( ViewType::YZ );
	m_pXYWnd->PositionView();
	Sys_UpdateWindows( W_XY );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */

BOOL DoMru( HWND hWnd, WORD wId )
{
	char szFileName[128];
	OFSTRUCT of;
	BOOL fExist;

	GetMenuItem( g_qeglobals.d_lpMruMenu, wId, TRUE, szFileName, sizeof( szFileName ) );

	// Test if the file exists.

	fExist = OpenFile( szFileName , &of, OF_EXIST ) != HFILE_ERROR;

	if( fExist )
	{

		// Place the file on the top of MRU.
		AddNewItem( g_qeglobals.d_lpMruMenu, ( LPSTR )szFileName );

		// Now perform opening this file !!!
		Map_LoadFile( szFileName );
	}
	else
		// Remove the file on MRU.
	{
		DelMenuItem( g_qeglobals.d_lpMruMenu, wId, TRUE );
	}

	// Refresh the File menu.
	PlaceMenuMRUItem( g_qeglobals.d_lpMruMenu, GetSubMenu( GetMenu( hWnd ), 0 ),
					  ID_FILE_EXIT );

	return fExist;
}

void CMainFrame::OnMru( unsigned int nID )
{
	// DHM - _D3XP
	if( ConfirmModified() )
	{
		DoMru( GetSafeHwnd(), nID );
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnViewNearest( unsigned int nID )
{
	Texture_SetMode( nID );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnViewShowblocks()
{
	g_qeglobals.show_blocks = !( g_qeglobals.show_blocks );
	CheckMenuItem
	(
		::GetMenu( GetSafeHwnd() ),
		ID_VIEW_SHOWBLOCKS,
		MF_BYCOMMAND | ( g_qeglobals.show_blocks ? MF_CHECKED : MF_UNCHECKED )
	);
	Sys_UpdateWindows( W_XY );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnViewShowclip()
{
	if( ( g_qeglobals.d_savedinfo.exclude ^= EXCLUDE_CLIP ) & EXCLUDE_CLIP )
	{
		CheckMenuItem( ::GetMenu( GetSafeHwnd() ), ID_VIEW_SHOWCLIP, MF_BYCOMMAND | MF_UNCHECKED );
	}
	else
	{
		CheckMenuItem( ::GetMenu( GetSafeHwnd() ), ID_VIEW_SHOWCLIP, MF_BYCOMMAND | MF_CHECKED );
	}

	Sys_UpdateWindows( W_XY | W_CAMERA );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnViewShowTriggers()
{
	if( ( g_qeglobals.d_savedinfo.exclude ^= EXCLUDE_TRIGGERS ) & EXCLUDE_TRIGGERS )
	{
		CheckMenuItem( ::GetMenu( GetSafeHwnd() ), ID_VIEW_SHOWTRIGGERS, MF_BYCOMMAND | MF_UNCHECKED );
	}
	else
	{
		CheckMenuItem( ::GetMenu( GetSafeHwnd() ), ID_VIEW_SHOWTRIGGERS, MF_BYCOMMAND | MF_CHECKED );
	}

	Sys_UpdateWindows( W_XY | W_CAMERA );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnViewShowcoordinates()
{
	g_qeglobals.d_savedinfo.show_coordinates ^= 1;
	CheckMenuItem
	(
		::GetMenu( GetSafeHwnd() ),
		ID_VIEW_SHOWCOORDINATES,
		MF_BYCOMMAND | ( g_qeglobals.d_savedinfo.show_coordinates ? MF_CHECKED : MF_UNCHECKED )
	);
	Sys_UpdateWindows( W_XY );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnViewShowent()
{
	if( ( g_qeglobals.d_savedinfo.exclude ^= EXCLUDE_ENT ) & EXCLUDE_ENT )
	{
		CheckMenuItem( ::GetMenu( GetSafeHwnd() ), ID_VIEW_SHOWENT, MF_BYCOMMAND | MF_UNCHECKED );
	}
	else
	{
		CheckMenuItem( ::GetMenu( GetSafeHwnd() ), ID_VIEW_SHOWENT, MF_BYCOMMAND | MF_CHECKED );
	}

	Sys_UpdateWindows( W_XY | W_CAMERA );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnViewShowlights()
{
	if( ( g_qeglobals.d_savedinfo.exclude ^= EXCLUDE_LIGHTS ) & EXCLUDE_LIGHTS )
	{
		CheckMenuItem( ::GetMenu( GetSafeHwnd() ), ID_VIEW_SHOWLIGHTS, MF_BYCOMMAND | MF_UNCHECKED );
	}
	else
	{
		CheckMenuItem( ::GetMenu( GetSafeHwnd() ), ID_VIEW_SHOWLIGHTS, MF_BYCOMMAND | MF_CHECKED );
	}

	Sys_UpdateWindows( W_XY | W_CAMERA );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnViewShownames()
{
	g_qeglobals.d_savedinfo.show_names = !( g_qeglobals.d_savedinfo.show_names );
	CheckMenuItem
	(
		::GetMenu( GetSafeHwnd() ),
		ID_VIEW_SHOWNAMES,
		MF_BYCOMMAND | ( g_qeglobals.d_savedinfo.show_names ? MF_CHECKED : MF_UNCHECKED )
	);
	Map_BuildBrushData();
	Sys_UpdateWindows( W_XY );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnViewShowpath()
{
	if( ( g_qeglobals.d_savedinfo.exclude ^= EXCLUDE_PATHS ) & EXCLUDE_PATHS )
	{
		CheckMenuItem( ::GetMenu( GetSafeHwnd() ), ID_VIEW_SHOWPATH, MF_BYCOMMAND | MF_UNCHECKED );
	}
	else
	{
		CheckMenuItem( ::GetMenu( GetSafeHwnd() ), ID_VIEW_SHOWPATH, MF_BYCOMMAND | MF_CHECKED );
	}

	Sys_UpdateWindows( W_XY | W_CAMERA );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnViewShowCombatNodes()
{
	if( ( g_qeglobals.d_savedinfo.exclude ^= EXCLUDE_COMBATNODES ) & EXCLUDE_COMBATNODES )
	{
		CheckMenuItem( ::GetMenu( GetSafeHwnd() ), ID_VIEW_SHOWCOMBATNODES, MF_BYCOMMAND | MF_UNCHECKED );
	}
	else
	{
		CheckMenuItem( ::GetMenu( GetSafeHwnd() ), ID_VIEW_SHOWCOMBATNODES, MF_BYCOMMAND | MF_CHECKED );
	}

	Sys_UpdateWindows( W_XY | W_CAMERA );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnViewShowwater()
{
	if( ( g_qeglobals.d_savedinfo.exclude ^= EXCLUDE_DYNAMICS ) & EXCLUDE_DYNAMICS )
	{
		CheckMenuItem( ::GetMenu( GetSafeHwnd() ), ID_VIEW_SHOWWATER, MF_BYCOMMAND | MF_UNCHECKED );
	}
	else
	{
		CheckMenuItem( ::GetMenu( GetSafeHwnd() ), ID_VIEW_SHOWWATER, MF_BYCOMMAND | MF_CHECKED );
	}

	Sys_UpdateWindows( W_XY | W_CAMERA );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnViewShowworld()
{
	if( ( g_qeglobals.d_savedinfo.exclude ^= EXCLUDE_WORLD ) & EXCLUDE_WORLD )
	{
		CheckMenuItem( ::GetMenu( GetSafeHwnd() ), ID_VIEW_SHOWWORLD, MF_BYCOMMAND | MF_UNCHECKED );
	}
	else
	{
		CheckMenuItem( ::GetMenu( GetSafeHwnd() ), ID_VIEW_SHOWWORLD, MF_BYCOMMAND | MF_CHECKED );
	}

	Sys_UpdateWindows( W_XY | W_CAMERA );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnViewUpfloor()
{
	m_pCamWnd->Cam_ChangeFloor( true );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnViewXy()
{
	m_pXYWnd->SetViewType( ViewType::XY );
	m_pXYWnd->PositionView();
	Sys_UpdateWindows( W_XY );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnViewZ100()
{
	z.scale = 1;
	Sys_UpdateWindows( W_Z | W_Z_OVERLAY );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnViewZoomin()
{
	if( m_pXYWnd && m_pXYWnd->Active() )
	{
		m_pXYWnd->SetScale( m_pXYWnd->Scale() * 5.0f / 4.0f );
		if( m_pXYWnd->Scale() > 256.0f )
		{
			m_pXYWnd->SetScale( 256.0f );
		}
	}

	if( m_pXZWnd && m_pXZWnd->Active() )
	{
		m_pXZWnd->SetScale( m_pXZWnd->Scale() * 5.0f / 4.0f );
		if( m_pXZWnd->Scale() > 256.0f )
		{
			m_pXZWnd->SetScale( 256.0f );
		}
	}

	if( m_pYZWnd && m_pYZWnd->Active() )
	{
		m_pYZWnd->SetScale( m_pYZWnd->Scale() * 5.0f / 4.0f );
		if( m_pYZWnd->Scale() > 256.0f )
		{
			m_pYZWnd->SetScale( 256.0f );
		}
	}

	Sys_UpdateWindows( W_XY | W_XY_OVERLAY );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnViewZoomout()
{
	if( m_pXYWnd && m_pXYWnd->Active() )
	{
		m_pXYWnd->SetScale( m_pXYWnd->Scale() * 4.0f / 5.0f );
		if( m_pXYWnd->Scale() < 0.1f / 32.0f )
		{
			m_pXYWnd->SetScale( 0.1f / 32.0f );
		}
	}

	if( m_pXZWnd && m_pXZWnd->Active() )
	{
		m_pXZWnd->SetScale( m_pXZWnd->Scale() * 4.0f / 5.0f );
		if( m_pXZWnd->Scale() < 0.1f / 32.0f )
		{
			m_pXZWnd->SetScale( 0.1f / 32.0f );
		}
	}

	if( m_pYZWnd && m_pYZWnd->Active() )
	{
		m_pYZWnd->SetScale( m_pYZWnd->Scale() * 4.0f / 5.0f );
		if( m_pYZWnd->Scale() < 0.1f / 32.0f )
		{
			m_pYZWnd->SetScale( 0.1f / 32.0f );
		}
	}

	Sys_UpdateWindows( W_XY | W_XY_OVERLAY );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnViewZzoomin()
{
	z.scale *= 5.0f / 4.0f;
	if( z.scale > 4.0f )
	{
		z.scale = 4.0f;
	}

	Sys_UpdateWindows( W_Z | W_Z_OVERLAY );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnViewZzoomout()
{
	z.scale *= 4.0f / 5.0f;
	if( z.scale < 0.125f )
	{
		z.scale = 0.125f;
	}

	Sys_UpdateWindows( W_Z | W_Z_OVERLAY );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnViewSide()
{
	m_pXYWnd->SetViewType( ViewType::XZ );
	m_pXYWnd->PositionView();
	Sys_UpdateWindows( W_XY );
}

static void UpdateGrid()
{
	// g_qeglobals.d_gridsize = 1 << g_qeglobals.d_gridsize;
	if( g_PrefsDlg.m_bSnapTToGrid )
	{
		g_qeglobals.d_savedinfo.m_nTextureTweak = g_qeglobals.d_gridsize;
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnGrid1( unsigned int nID )
{
	switch( nID )
	{
		case ID_GRID_1:
			g_qeglobals.d_gridsize = 1;
			break;
		case ID_GRID_2:
			g_qeglobals.d_gridsize = 2;
			break;
		case ID_GRID_4:
			g_qeglobals.d_gridsize = 4;
			break;
		case ID_GRID_8:
			g_qeglobals.d_gridsize = 8;
			break;
		case ID_GRID_16:
			g_qeglobals.d_gridsize = 16;
			break;
		case ID_GRID_32:
			g_qeglobals.d_gridsize = 32;
			break;
		case ID_GRID_64:
			g_qeglobals.d_gridsize = 64;
			break;
		case ID_GRID_POINT5:
			g_qeglobals.d_gridsize = 0.5f;
			break;
		case ID_GRID_POINT25:
			g_qeglobals.d_gridsize = 0.25f;
			break;
		case ID_GRID_POINT125:
			g_qeglobals.d_gridsize = 0.125f;
			break;
			//case ID_GRID_POINT0625:
			//	g_qeglobals.d_gridsize = 0.0625f;
			//	break;
	}

	UpdateGrid();

	SetGridStatus();
	SetGridChecks( nID );
	Sys_UpdateWindows( W_XY | W_Z | W_CAMERA );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnTexturesShowinuse()
{
	Sys_BeginWait();
	Texture_ShowInuse();
	g_Inspectors->texWnd.RedrawWindow();
}

// from TexWnd.cpp
extern bool texture_showinuse;

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnUpdateTexturesShowinuse( CCmdUI* pCmdUI )
{
	pCmdUI->SetCheck( texture_showinuse );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnTexturesInspector()
{
	DoSurface();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnMiscFindbrush()
{
	CFindBrushDlg dlg;
	dlg.DoModal();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnMiscNextleakspot()
{
	Pointfile_Next();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnMiscPreviousleakspot()
{
	Pointfile_Prev();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */

void UpdateRadiantColor( float r, float g, float b, float a )
{
	if( g_pParentWnd )
	{
		g_pParentWnd->RoutineProcessing();
	}
}

bool DoColor( int iIndex )
{
	COLORREF cr = ( int )( g_qeglobals.d_savedinfo.colors[iIndex][0] * 255 ) +
				  ( ( ( int )( g_qeglobals.d_savedinfo.colors[iIndex][1] * 255 ) ) << 8 ) +
				  ( ( ( int )( g_qeglobals.d_savedinfo.colors[iIndex][2] * 255 ) ) << 16 );

	CDialogColorPicker dlg( cr );

	dlg.UpdateParent = UpdateRadiantColor;

	if( dlg.DoModal() == IDOK )
	{
		g_qeglobals.d_savedinfo.colors[iIndex][0] = ( dlg.GetColor() & 255 ) / 255.0;
		g_qeglobals.d_savedinfo.colors[iIndex][1] = ( ( dlg.GetColor() >> 8 ) & 255 ) / 255.0;
		g_qeglobals.d_savedinfo.colors[iIndex][2] = ( ( dlg.GetColor() >> 16 ) & 255 ) / 255.0;

		Sys_UpdateWindows( W_ALL );
		return true;
	}
	else
	{
		return false;
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
extern void Select_SetKeyVal( const char* key, const char* val );
void CMainFrame::OnMiscSelectentitycolor()
{

	idEditorEntity* ent = NULL;
	if( QE_SingleBrush( true, true ) )
	{
		ent = selected_brushes.next->owner;
		CString strColor = ent->ValueForKey( "_color" );
		if( strColor.GetLength() > 0 )
		{
			float	fR, fG, fB;
			int		n = sscanf( strColor, "%f %f %f", &fR, &fG, &fB );
			if( n == 3 )
			{
				g_qeglobals.d_savedinfo.colors[COLOR_ENTITY][0] = fR;
				g_qeglobals.d_savedinfo.colors[COLOR_ENTITY][1] = fG;
				g_qeglobals.d_savedinfo.colors[COLOR_ENTITY][2] = fB;
			}
		}
	}

	if( DoColor( COLOR_ENTITY ) )
	{
		char	buffer[100];
		sprintf( buffer, "%f %f %f", g_qeglobals.d_savedinfo.colors[COLOR_ENTITY][0], g_qeglobals.d_savedinfo.colors[COLOR_ENTITY][1], g_qeglobals.d_savedinfo.colors[COLOR_ENTITY][2] );
		Select_SetKeyVal( "_color", buffer );
		if( ent )
		{
			g_Inspectors->UpdateEntitySel( ent->eclass );
		}
		Sys_UpdateWindows( W_ALL );
	}
}

CString strFindKey;
CString strFindValue;
CString strReplaceKey;
CString strReplaceValue;
bool    gbWholeStringMatchOnly = true;
bool	gbSelectAllMatchingEnts = false;
idEditorBrush* gpPrevEntBrushFound = NULL;

// all this because there's no ansi stristr(), sigh...
//
LPCSTR String_ToLower( LPCSTR psString )
{
	const int iBufferSize = 4096;
	static char sString[8][iBufferSize];
	static int iIndex = 0;

	if( strlen( psString ) >= iBufferSize )
	{
		assert( 0 );
		common->Printf( "String_ToLower(): Warning, input string was %d bytes too large, performing strlwr() inline!\n", strlen( psString ) - ( iBufferSize - 1 ) );
		return strlwr( const_cast<char*>( psString ) );
	}

	iIndex = ++ iIndex & 7;

	strcpy( sString[iIndex], psString );
	strlwr( sString[iIndex] );

	return sString[iIndex];
}


bool FindNextBrush( idEditorBrush* pPrevFoundBrush )	// can be NULL for fresh search
{
	bool bFoundSomething = false;
	idEditorEntity* pLastFoundEnt = NULL;
	idEditorBrush*  pLastFoundBrush = NULL;

	CWaitCursor waitcursor;

	Select_Deselect( true );	// bool bDeSelectToListBack

	// see whether to start search from prev_brush->next by checking if prev_brush is still in the active list...
	//
	idEditorBrush* pStartBrush = active_brushes.next;

	if( pPrevFoundBrush && !gbSelectAllMatchingEnts )
	{
		idEditorBrush* pPrev = NULL;
		for( idEditorBrush* b = active_brushes.next ; b != &active_brushes ; b = b->next )
		{
			if( pPrev == pPrevFoundBrush && pPrevFoundBrush )
			{
				pStartBrush = b;
				break;
			}
			pPrev = b;
		}
	}

	// now do the search proper...
	//
	int iBrushesScanned = 0;
	int iBrushesSelected = 0;
	int iEntsScanned = 0;

	idEditorBrush* pNextBrush;
	for( idEditorBrush* b = pStartBrush; b != &active_brushes ; b = pNextBrush )
	{
		// setup the <nextbrush> ptr before going any further (because selecting a brush down below moves it to a
		//	different link list), but we need to ensure that the next brush has a different ent-owner than the current
		//	one, or multi-brush ents will confuse the list process if they get selected (infinite loop badness)...
		//
		// pNextBrush = &active_brushes;	// default to loop-stop condition
		pNextBrush = b->next;
		while( pNextBrush->owner == b->owner && pNextBrush != &active_brushes )
		{
			pNextBrush = pNextBrush->next;
		}

		iBrushesScanned++;

		// a simple progress bar so they don't think it's locked up on long searches...
		//
		static int iDotBodge = 0;
		if( !( ++iDotBodge & 15 ) )
		{
			common->Printf( "." );    // cut down on printing
		}

		bool bMatch = false;
		idEditorEntity* ent = b->owner;

		if( ent && ent != world_entity )	// needed!
		{
			iEntsScanned++;
			if( FilterBrush( b ) )
			{
				continue;
			}

			// only check the find-key if there was one specified...
			//
			if( !strFindKey.IsEmpty() )
			{
				const char* psEntFoundValue = ent->ValueForKey( strFindKey );

				if( strlen( psEntFoundValue )
						&&
						(
//							(stricmp(strFindValue, psEntFoundValue)==0)	// found this exact key/value
							(
								( gbWholeStringMatchOnly && stricmp( psEntFoundValue, strFindValue ) == 0 )
								||
								( !gbWholeStringMatchOnly && strstr( String_ToLower( psEntFoundValue ), String_ToLower( strFindValue ) ) )
							)
							||											//  or
							( strFindValue.IsEmpty() )					// any value for this key if blank value search specified
						)
				  )
				{
					bMatch = true;
				}
			}
			else
			{
				// no FIND key specified, so just scan all of them...
				//
				int iNumEntKeys = ent->GetNumKeys();
				for( int i = 0; i < iNumEntKeys; i++ )
				{
					const char* psEntFoundValue = ent->ValueForKey( ent->GetKeyString( i ) );
					if( psEntFoundValue )
					{
						if(	( strlen( psEntFoundValue ) &&	strFindValue.IsEmpty() )	// if blank <value> search specified then any found-value is ok
								||
								( gbWholeStringMatchOnly && stricmp( psEntFoundValue, strFindValue ) == 0 )
								||
								( !gbWholeStringMatchOnly && strstr( String_ToLower( psEntFoundValue ), String_ToLower( strFindValue ) ) )
						  )
						{
							if( !gbWholeStringMatchOnly && strstr( String_ToLower( psEntFoundValue ), String_ToLower( strFindValue ) ) )
							{
//								OutputDebugString(va("Matching because: psEntFoundValue '%s' & strFindValue '%s'\n",psEntFoundValue, strFindValue));
//								Sys_Printf("Matching because: psEntFoundValue '%s' & strFindValue '%s'\n",psEntFoundValue, strFindValue);

//								if (strstr(psEntFoundValue,"killsplat"))
//								{
//									DebugBreak();
//								}
							}
							bMatch = true;
							break;
						}
					}
				}
			}

			if( bMatch )
			{
				bFoundSomething = true;
				pLastFoundEnt	= ent;
				pLastFoundBrush	= b;
				iBrushesSelected++;

				g_bScreenUpdates = false;	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!

				Select_Brush( b );

				g_bScreenUpdates = true;	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!

				if( !gbSelectAllMatchingEnts )
				{
					break;
				}
			}
		}
	}
	if( gbSelectAllMatchingEnts )
	{
		common->Printf( "\nBrushes Selected: %d           (Brushes Scanned %d, Ents Scanned %d)\n", iBrushesSelected, iBrushesScanned, iEntsScanned );
	}

	if( bFoundSomething )
	{
		idVec3 v3Origin;

		if( pLastFoundEnt->origin[0] != 0.0f || pLastFoundEnt->origin[1] != 0.0f || pLastFoundEnt->origin[2] != 0.0f )
		{
			VectorCopy( pLastFoundEnt->origin, v3Origin );
		}
		else
		{
			// pLastFoundEnt's origin is zero, so use average point of brush mins maxs instead...
			//
			v3Origin[0] = ( pLastFoundBrush->mins[0] + pLastFoundBrush->maxs[0] ) / 2;
			v3Origin[1] = ( pLastFoundBrush->mins[1] + pLastFoundBrush->maxs[1] ) / 2;
			v3Origin[2] = ( pLastFoundBrush->mins[2] + pLastFoundBrush->maxs[2] ) / 2;
		}

		// got one, jump the camera to it...
		//
		VectorCopy( v3Origin, g_pParentWnd->GetCamera()->Camera().origin );
		g_pParentWnd->GetCamera()->Camera().origin[1] -= 32;	// back off a touch to look at it
		g_pParentWnd->GetCamera()->Camera().angles[0] = 0;
		g_pParentWnd->GetCamera()->Camera().angles[1] = 90;
		g_pParentWnd->GetCamera()->Camera().angles[2] = 0;

		// force main screen into XY camera mode (just in case)...
		//
		g_pParentWnd->SetActiveXY( g_pParentWnd->GetXYWnd() );
		g_pParentWnd->GetXYWnd()->PositionView();

		Sys_UpdateWindows( W_ALL );
		//
		// and record for next find request (F3)...
		//
		gpPrevEntBrushFound = pLastFoundBrush;
	}

	return bFoundSomething;
}


void CMainFrame::OnMiscFindOrReplaceEntity()
{
	CEntKeyFindReplace FindReplace( &strFindKey, &strFindValue, &strReplaceKey, &strReplaceValue, &gbWholeStringMatchOnly, &gbSelectAllMatchingEnts );
	switch( FindReplace.DoModal() )
	{
		case ID_RET_REPLACE:
		{
			idEditorBrush* next = NULL;
			int iOccurences = 0;
			for( idEditorBrush* b = active_brushes.next ; b != &active_brushes ; b = next )
			{
				next = b->next;	// important to do this here, in case brush gets linked to a different list
				idEditorEntity* ent = b->owner;

				if( ent )	// needed!
				{
					if( FilterBrush( b ) )
					{
						continue;
					}

					const char* psEntFoundValue = ent->ValueForKey( strFindKey );

					if( stricmp( strFindValue, psEntFoundValue ) == 0 ||		// found this exact key/value
							( strlen( psEntFoundValue ) &&	strFindValue.IsEmpty() ) // or any value for this key if blank value search specified
					  )
					{
						// found this search key/value, so delete it...
						//
						ent->DeleteKey( strFindKey );
						//
						// and replace with the new key/value (if specified)...
						//
						if( !strReplaceKey.IsEmpty() && !strReplaceValue.IsEmpty() )
						{
							ent->SetKeyValue( strReplaceKey, strReplaceValue );
						}
						iOccurences++;
					}
				}
			}
			if( iOccurences )
			{
				common->Printf( "%d occurence(s) replaced\n", iOccurences );
			}
			else
			{
				common->Printf( "Nothing found to replace\n" );
			}
		}
		break;
		case ID_RET_FIND:
		{
			gpPrevEntBrushFound = NULL;
			FindNextBrush( NULL );
		}
		break;
	}
}
void CMainFrame::OnMiscFindNextEntity()
{
	// try it once, if it fails, try it again from top, and give up if still failed after that...
	//
	if( !FindNextBrush( gpPrevEntBrushFound ) )
	{
		gpPrevEntBrushFound = NULL;
		FindNextBrush( NULL );
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnTexturebk()
{
	DoColor( COLOR_TEXTUREBACK );
	Sys_UpdateWindows( W_ALL );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnColorsMajor()
{
	DoColor( COLOR_GRIDMAJOR );
	Sys_UpdateWindows( W_ALL );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnColorsMinor()
{
	DoColor( COLOR_GRIDMINOR );
	Sys_UpdateWindows( W_ALL );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnColorsXybk()
{
	DoColor( COLOR_GRIDBACK );
	Sys_UpdateWindows( W_ALL );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnColorsCameraBk()
{
	DoColor( COLOR_CAMERABACK );
	Sys_UpdateWindows( W_CAMERA );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnBrush3sided()
{
	Undo_Start( "3 sided" );
	Undo_AddBrushList( &selected_brushes );
	Brush_MakeSided( 3 );
	Undo_EndBrushList( &selected_brushes );
	Undo_End();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnBrush4sided()
{
	Undo_Start( "4 sided" );
	Undo_AddBrushList( &selected_brushes );
	Brush_MakeSided( 4 );
	Undo_EndBrushList( &selected_brushes );
	Undo_End();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnBrush5sided()
{
	Undo_Start( "5 sided" );
	Undo_AddBrushList( &selected_brushes );
	Brush_MakeSided( 5 );
	Undo_EndBrushList( &selected_brushes );
	Undo_End();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnBrush6sided()
{
	Undo_Start( "6 sided" );
	Undo_AddBrushList( &selected_brushes );
	Brush_MakeSided( 6 );
	Undo_EndBrushList( &selected_brushes );
	Undo_End();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnBrush7sided()
{
	Undo_Start( "7 sided" );
	Undo_AddBrushList( &selected_brushes );
	Brush_MakeSided( 7 );
	Undo_EndBrushList( &selected_brushes );
	Undo_End();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnBrush8sided()
{
	Undo_Start( "8 sided" );
	Undo_AddBrushList( &selected_brushes );
	Brush_MakeSided( 8 );
	Undo_EndBrushList( &selected_brushes );
	Undo_End();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnBrush9sided()
{
	Undo_Start( "9 sided" );
	Undo_AddBrushList( &selected_brushes );
	Brush_MakeSided( 9 );
	Undo_EndBrushList( &selected_brushes );
	Undo_End();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnBrushArbitrarysided()
{
	Undo_Start( "arbitrary sided" );
	Undo_AddBrushList( &selected_brushes );
	DoSides();
	Undo_EndBrushList( &selected_brushes );
	Undo_End();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnBrushFlipx()
{
	Undo_Start( "flip X" );
	Undo_AddBrushList( &selected_brushes );

	Select_FlipAxis( 0 );
	for( idEditorBrush* b = selected_brushes.next; b != &selected_brushes; b = b->next )
	{
		if( b->owner->eclass->fixedsize )
		{
			char	buf[16];
			float a = b->owner->FloatForKey( "angle" );
			a = div( ( 180 - a ), 180 ).rem;
			b->owner->SetKeyValue( "angle", itoa( a, buf, 10 ) );
			Brush_Build( b );
		}
	}
	Patch_ToggleInverted();
	Undo_EndBrushList( &selected_brushes );
	Undo_End();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnBrushFlipy()
{
	Undo_Start( "flip Y" );
	Undo_AddBrushList( &selected_brushes );

	Select_FlipAxis( 1 );
	for( idEditorBrush* b = selected_brushes.next; b != &selected_brushes; b = b->next )
	{
		if( b->owner->eclass->fixedsize )
		{
			float a = b->owner->FloatForKey( "angle" );
			if( a == 0 || a == 180 || a == 360 )
			{
				continue;
			}

			if( a == 90 || a == 270 )
			{
				a += 180;
			}
			else if( a > 270 )
			{
				a += 90;
			}
			else if( a > 180 )
			{
				a -= 90;
			}
			else if( a > 90 )
			{
				a += 90;
			}
			else
			{
				a -= 90;
			}

			a = ( int )a % 360;

			char	buf[16];
			b->owner->SetKeyValue( "angle", itoa( a, buf, 10 ) );
			Brush_Build( b );
		}
	}
	Patch_ToggleInverted();
	Undo_EndBrushList( &selected_brushes );
	Undo_End();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnBrushFlipz()
{
	Undo_Start( "flip Z" );
	Undo_AddBrushList( &selected_brushes );
	Select_FlipAxis( 2 );
	Patch_ToggleInverted();
	Undo_EndBrushList( &selected_brushes );
	Undo_End();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnBrushRotatex()
{
	Undo_Start( "rotate X" );
	Undo_AddBrushList( &selected_brushes );
	Select_RotateAxis( 0, 90 );
	Undo_EndBrushList( &selected_brushes );
	Undo_End();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnBrushRotatey()
{
	Undo_Start( "rotate Y" );
	Undo_AddBrushList( &selected_brushes );
	Select_RotateAxis( 1, 90 );
	Undo_EndBrushList( &selected_brushes );
	Undo_End();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnBrushRotatez()
{
	Undo_Start( "rotate Z" );
	Undo_AddBrushList( &selected_brushes );
	Select_RotateAxis( 2, 90 );
	Undo_EndBrushList( &selected_brushes );
	Undo_End();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnRegionOff()
{
	Map_RegionOff();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnRegionSetbrush()
{
	Map_RegionBrush();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnRegionSetselection()
{
	Map_RegionSelectedBrushes();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnRegionSettallbrush()
{
	Map_RegionTallBrush();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnRegionSetxy()
{
	Map_RegionXY();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnSelectionArbitraryrotation()
{
	// if (ActiveXY()) ActiveXY()->UndoCopy();
	Undo_Start( "arbitrary rotation" );
	Undo_AddBrushList( &selected_brushes );

	CRotateDlg	dlg;
	dlg.DoModal();

	Undo_EndBrushList( &selected_brushes );
	Undo_End();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnSelectionClone()
{
	// if (ActiveXY()) ActiveXY()->UndoCopy();
	Select_Clone();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnSelectionConnect()
{
	ConnectEntities();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnSelectionMakehollow()
{
	// if (ActiveXY()) ActiveXY()->UndoCopy();
	Undo_Start( "hollow" );
	Undo_AddBrushList( &selected_brushes );
	CSG_MakeHollow();
	Undo_EndBrushList( &selected_brushes );
	Undo_End();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnSelectionCsgsubtract()
{
	// if (ActiveXY()) ActiveXY()->UndoCopy();
	Undo_Start( "CSG subtract" );
	CSG_Subtract();
	Undo_End();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnSelectionCsgmerge()
{
	// if (ActiveXY()) ActiveXY()->UndoCopy();
	Undo_Start( "CSG merge" );
	Undo_AddBrushList( &selected_brushes );
	CSG_Merge();
	Undo_EndBrushList( &selected_brushes );
	Undo_End();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnSelectionDelete()
{
	idEditorBrush* brush;

	// if (ActiveXY()) ActiveXY()->UndoCopy();
	Undo_Start( "delete" );
	Undo_AddBrushList( &selected_brushes );

	// add all deleted entities to the undo
	for( brush = selected_brushes.next; brush != &selected_brushes; brush = brush->next )
	{
		Undo_AddEntity( brush->owner );
	}

	// NOTE: Select_Delete does NOT delete entities
	Select_Delete();
	Undo_EndBrushList( &selected_brushes );
	Undo_End();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnSelectionDeselect()
{
	if( !ByeByeSurfaceDialog() )
	{
		if( g_bClipMode )
		{
			OnViewClipper();
		}
		else if( g_bRotateMode )
		{
			OnSelectMouserotate();
		}
		else if( g_bScaleMode )
		{
			OnSelectMousescale();
		}
		else if( g_bPathMode )
		{
			if( ActiveXY() )
			{
				ActiveXY()->KillPathMode();
			}
		}
		else if( g_bAxialMode )
		{
			g_bAxialMode = false;
			Sys_UpdateWindows( W_CAMERA );
		}
		else
		{
			if( g_qeglobals.d_select_mode == sel_curvepoint && g_qeglobals.d_num_move_points > 0 )
			{
				g_qeglobals.d_num_move_points = 0;
				Sys_UpdateWindows( W_ALL );
			}
			else
			{
				Select_Deselect();
				SetStatusText( 2, " " );
			}
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnSelectionDragedges()
{
	if( g_qeglobals.d_select_mode == sel_edge )
	{
		g_qeglobals.d_select_mode = sel_brush;
		Sys_UpdateWindows( W_ALL );
	}
	else
	{
		SetupVertexSelection();
		if( g_qeglobals.d_numpoints )
		{
			g_qeglobals.d_select_mode = sel_edge;
		}

		Sys_UpdateWindows( W_ALL );
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnSelectionDragvertecies()
{
	if( g_qeglobals.d_select_mode == sel_vertex || g_qeglobals.d_select_mode == sel_curvepoint )
	{
		g_qeglobals.d_select_mode = sel_brush;
		Sys_UpdateWindows( W_ALL );
	}
	else
	{
		// --if (QE_SingleBrush() && selected_brushes.next->patchBrush)
		if( OnlyPatchesSelected() )
		{
			Patch_EditPatch();
		}
		else if( !AnyPatchesSelected() )
		{
			SetupVertexSelection();
			if( g_qeglobals.d_numpoints )
			{
				g_qeglobals.d_select_mode = sel_vertex;
			}
		}

		Sys_UpdateWindows( W_ALL );
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnSelectionCenterOrigin()
{
	Undo_Start( "center origin" );
	Undo_AddBrushList( &selected_brushes );
	Select_CenterOrigin();
	Undo_EndBrushList( &selected_brushes );
	Undo_End();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnSelectionSelectcompletetall()
{
	//if (ActiveXY()) {
	//	ActiveXY()->UndoCopy();
	//}

	Select_CompleteTall();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnSelectionSelectinside()
{
	Select_Inside();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnSelectionSelectpartialtall()
{
	Select_PartialTall();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnSelectionSelecttouching()
{
	Select_Touching();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnSelectionUngroupentity()
{
	Select_Ungroup();
}

/*
=======================================================================================================================
=======================================================================================================================
*/
void CMainFrame::OnAutocaulk()
{
	Select_AutoCaulk();
}

/*
=======================================================================================================================
=======================================================================================================================
*/
void CMainFrame::OnUpdateAutocaulk( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( selected_brushes.next != &selected_brushes );
}


/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnTexturesPopup()
{
	HandlePopup( this, IDR_POPUP_TEXTURE );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnSplinesPopup()
{
	HandlePopup( this, IDR_POPUP_SPLINE );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnPopupSelection()
{
	HandlePopup( this, IDR_POPUP_SELECTION );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnViewChange()
{
	OnViewNextview();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnViewCameraupdate()
{
	g_qeglobals.flatRotation++;

	if( g_qeglobals.flatRotation > 2 )
	{
		g_qeglobals.flatRotation = 0;
	}

	if( g_qeglobals.flatRotation )
	{
		g_qeglobals.rotateAxis = 0;
		if( ActiveXY()->GetViewType() == ViewType::XY )
		{
			g_qeglobals.rotateAxis = 2;
		}
		else if( ActiveXY()->GetViewType() == ViewType::XZ )
		{
			g_qeglobals.rotateAxis = 1;
		}
	}
	Select_InitializeRotation();
	Sys_UpdateWindows( W_CAMERA | W_XY );
}


/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnSizing( UINT fwSide, LPRECT pRect )
{
	CFrameWndEx::OnSizing( fwSide, pRect );
	GetClientRect( g_rctOld );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnViewClipper()
{
	if( ActiveXY() )
	{
		if( ActiveXY()->ClipMode() )
		{
			ActiveXY()->SetClipMode( false );
			SetToolbarButtonCheckState( m_wndToolBar, ID_VIEW_CLIPPER, FALSE );
		}
		else
		{
			if( ActiveXY()->RotateMode() )
			{
				OnSelectMouserotate();
			}

			ActiveXY()->SetClipMode( true );
			SetToolbarButtonCheckState( m_wndToolBar, ID_VIEW_CLIPPER, TRUE );
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnCameraAngledown()
{
	m_pCamWnd->Camera().angles[0] -= SPEED_TURN;
	if( m_pCamWnd->Camera().angles[0] < -85 )
	{
		m_pCamWnd->Camera().angles[0] = -85;
	}

	Sys_UpdateWindows( W_CAMERA | W_XY_OVERLAY );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnCameraAngleup()
{
	m_pCamWnd->Camera().angles[0] += SPEED_TURN;
	if( m_pCamWnd->Camera().angles[0] > 85 )
	{
		m_pCamWnd->Camera().angles[0] = 85;
	}

	Sys_UpdateWindows( W_CAMERA | W_XY_OVERLAY );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnCameraBack()
{
	// Calculate delta time
	auto currentFrameTime = std::chrono::steady_clock::now();
	std::chrono::duration<float> deltaTime = currentFrameTime - lastFrameTime;
	lastFrameTime = std::chrono::steady_clock::now();
	float deltaTimeSeconds = std::min( deltaTime.count(), 0.1f );

	// Calculate movement based on delta time
	float moveAmount = radiant_cameraMoveSpeed.GetFloat() * deltaTimeSeconds;

	// Get the camera direction vectors
	idVec3 forward = m_pCamWnd->Camera().forward;
	idVec3 right = m_pCamWnd->Camera().right;
	idVec3 up = idVec3( 0, 0, 1 );

	// Update camera origin with smooth movement
	m_pCamWnd->Camera().origin -= forward * moveAmount;

	// Determine which windows to update
	Sys_UpdateWindows( W_CAMERA | W_XY );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnCameraDown()
{
	m_pCamWnd->Camera().origin[2] -= SPEED_MOVE;
	Sys_UpdateWindows( W_CAMERA | W_XY | W_Z );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnCameraForward()
{
	// Calculate delta time
	auto currentFrameTime = std::chrono::steady_clock::now();
	std::chrono::duration<float> deltaTime = currentFrameTime - lastFrameTime;
	lastFrameTime = std::chrono::steady_clock::now();
	float deltaTimeSeconds = std::min( deltaTime.count(), 0.1f );

	// Calculate movement based on delta time
	float moveAmount = radiant_cameraMoveSpeed.GetFloat() * deltaTimeSeconds;

	// Get the camera direction vectors
	idVec3 forward = m_pCamWnd->Camera().forward;
	idVec3 right = m_pCamWnd->Camera().right;
	idVec3 up = idVec3( 0, 0, 1 );

	// Update camera origin with smooth movement
	m_pCamWnd->Camera().origin += forward * moveAmount;

	// Determine which windows to update
	Sys_UpdateWindows( W_CAMERA | W_XY );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnCameraLeft()
{
	// Calculate delta time
	auto currentFrameTime = std::chrono::steady_clock::now();
	std::chrono::duration<float> deltaTime = currentFrameTime - lastFrameTime;
	lastFrameTime = std::chrono::steady_clock::now();
	float deltaTimeSeconds = std::min( deltaTime.count(), 0.1f );

	// Calculate movement based on delta time
	float moveAmount = radiant_cameraMoveSpeed.GetFloat() * deltaTimeSeconds;

	// Get the camera direction vectors
	idVec3 forward = m_pCamWnd->Camera().forward;
	idVec3 right = m_pCamWnd->Camera().right;
	idVec3 up = idVec3( 0, 0, 1 );

	// Update camera origin with smooth movement
	m_pCamWnd->Camera().origin += right * moveAmount;

	// Determine which windows to update
	Sys_UpdateWindows( W_CAMERA | W_XY );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnCameraRight()
{
	// Calculate delta time
	auto currentFrameTime = std::chrono::steady_clock::now();
	std::chrono::duration<float> deltaTime = currentFrameTime - lastFrameTime;
	lastFrameTime = std::chrono::steady_clock::now();
	float deltaTimeSeconds = std::min( deltaTime.count(), 0.1f );

	// Calculate movement based on delta time
	float moveAmount = radiant_cameraMoveSpeed.GetFloat() * deltaTimeSeconds;

	// Get the camera direction vectors
	idVec3 forward = m_pCamWnd->Camera().forward;
	idVec3 right = m_pCamWnd->Camera().right;
	idVec3 up = idVec3( 0, 0, 1 );

	// Update camera origin with smooth movement
	m_pCamWnd->Camera().origin -= right * moveAmount;

	// Determine which windows to update
	Sys_UpdateWindows( W_CAMERA | W_XY );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnCameraStrafeleft()
{
	VectorMA( m_pCamWnd->Camera().origin, -SPEED_MOVE, m_pCamWnd->Camera().right, m_pCamWnd->Camera().origin );

	Sys_UpdateWindows( W_CAMERA | W_XY );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnCameraStraferight()
{
	VectorMA( m_pCamWnd->Camera().origin, SPEED_MOVE, m_pCamWnd->Camera().right, m_pCamWnd->Camera().origin );

	Sys_UpdateWindows( W_CAMERA | W_XY );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnCameraUp()
{
	m_pCamWnd->Camera().origin[2] += SPEED_MOVE;
	Sys_UpdateWindows( W_CAMERA | W_XY | W_Z );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnGridToggle()
{
	g_qeglobals.d_showgrid ^= 1;
	Sys_UpdateWindows( W_XY | W_Z );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnPrefs()
{
	g_PrefsDlg.LoadPrefs();
	if( g_PrefsDlg.DoModal() == IDOK )
	{
		g_Inspectors->texWnd.UpdatePrefs();

		CMenu*	pMenu = GetMenu();
		if( pMenu )
		{
			pMenu->CheckMenuItem( ID_SNAPTOGRID, MF_BYCOMMAND | ( !g_PrefsDlg.m_bNoClamp ) ? MF_CHECKED : MF_UNCHECKED );
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnTogglecamera()
{
	if( m_pCamWnd->IsWindowVisible() )
	{
		m_pCamWnd->ShowWindow( SW_HIDE );
	}
	else
	{
		m_pCamWnd->ShowWindow( SW_SHOW );
	}
}


/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnToggleview()
{
	if( m_pXYWnd && m_pXYWnd->GetSafeHwnd() )
	{
		if( m_pXYWnd->IsWindowVisible() )
		{
			m_pXYWnd->ShowWindow( SW_HIDE );
		}
		else
		{
			m_pXYWnd->ShowWindow( SW_SHOW );
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnTogglez()
{
	if( m_pZWnd && m_pZWnd->GetSafeHwnd() )
	{
		if( m_pZWnd->IsWindowVisible() )
		{
			m_pZWnd->ShowWindow( SW_HIDE );
		}
		else
		{
			m_pZWnd->ShowWindow( SW_SHOW );
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnToggleLock()
{
	g_PrefsDlg.m_bTextureLock = !g_PrefsDlg.m_bTextureLock;

	CMenu*	pMenu = GetMenu();
	if( pMenu )
	{
		pMenu->CheckMenuItem( ID_TOGGLE_LOCK, MF_BYCOMMAND | ( g_PrefsDlg.m_bTextureLock ) ? MF_CHECKED : MF_UNCHECKED );
	}

	g_PrefsDlg.SavePrefs();
	SetGridStatus();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnEditMapinfo()
{
	CMapInfo	dlg;
	dlg.DoModal();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnEditEntityinfo()
{
	CEntityListDlg::ShowDialog();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnViewNextview()
{
	if( m_pXYWnd->GetViewType() == ViewType::XY )
	{
		m_pXYWnd->SetViewType( ViewType::XZ );
	}
	else if( m_pXYWnd->GetViewType() == ViewType::XZ )
	{
		m_pXYWnd->SetViewType( ViewType::YZ );
	}
	else
	{
		m_pXYWnd->SetViewType( ViewType::XY );
	}

	m_pXYWnd->PositionView();
	if( g_qeglobals.flatRotation )
	{
		g_qeglobals.rotateAxis = 0;
		if( ActiveXY()->GetViewType() == ViewType::XY )
		{
			g_qeglobals.rotateAxis = 2;
		}
		else if( ActiveXY()->GetViewType() == ViewType::XZ )
		{
			g_qeglobals.rotateAxis = 1;
		}
	}
	Sys_UpdateWindows( W_XY | W_CAMERA );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnHelpCommandlist()
{
	CCommandsDlg	dlg;
	dlg.DoModal();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::UpdateStatusText()
{
	for( int n = 0; n < 6; n++ )
	{
		if( m_strStatus[n].GetLength() >= 0 && m_wndStatusBar.GetSafeHwnd() )
		{
			m_wndStatusBar.SetPaneText( n, m_strStatus[n] );
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::SetStatusText( int nPane, const char* pText )
{
	if( pText && nPane <= 5 && nPane >= 0 )
	{
		m_strStatus[nPane] = pText;
		UpdateStatusText();
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::UpdateWindows( int nBits )
{

	if( !g_bScreenUpdates )
	{
		return;
	}

	if( nBits & ( W_XY | W_XY_OVERLAY ) )
	{
		if( m_pXYWnd )
		{
			m_pXYWnd->RedrawWindow( NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW );
		}

		if( m_pXZWnd )
		{
			m_pXZWnd->RedrawWindow( NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW );
		}

		if( m_pYZWnd )
		{
			m_pYZWnd->RedrawWindow( NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW );
		}
	}

	if( nBits & W_CAMERA || ( ( nBits & W_CAMERA_ICON ) && m_bCamPreview ) )
	{
		if( m_pCamWnd )
		{
			m_pCamWnd->RedrawWindow( NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW );
		}
	}

	if( nBits & ( W_Z | W_Z_OVERLAY ) )
	{
		if( m_pZWnd )
		{
			m_pZWnd->RedrawWindow( NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW );
		}
	}

	if( nBits & W_TEXTURE )
	{
		g_Inspectors->texWnd.RedrawWindow( NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW );
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Sys_UpdateWindows( int nBits )
{
	if( g_pParentWnd )
	{
		g_pParentWnd->UpdateWindows( nBits );
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnFlipClip()
{
	if( m_pActiveXY )
	{
		m_pActiveXY->FlipClip();
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnClipSelected()
{
	if( m_pActiveXY && m_pActiveXY->ClipMode() )
	{
		Undo_Start( "clip selected" );
		Undo_AddBrushList( &selected_brushes );
		m_pActiveXY->Clip();
		Undo_EndBrushList( &selected_brushes );
		Undo_End();
	}
	else
	{
		if( g_bPatchBendMode )
		{
			Patch_BendHandleENTER();
		}
		else if( g_bAxialMode )
		{

		}
		//else if (g_bPatchBendMode) {
		//	Patch_InsDelHandleENTER();
		//}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnSplitSelected()
{
	if( m_pActiveXY )
	{
		Undo_Start( "split selected" );
		Undo_AddBrushList( &selected_brushes );
		m_pActiveXY->SplitClip();
		Undo_EndBrushList( &selected_brushes );
		Undo_End();
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
CXYWnd* CMainFrame::ActiveXY()
{
	return m_pActiveXY;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnToggleviewXz()
{
	if( m_pXZWnd && m_pXZWnd->GetSafeHwnd() )
	{
		// get windowplacement doesn't actually save this so we will here
		g_PrefsDlg.m_bXZVis = m_pXZWnd->IsWindowVisible();
		if( g_PrefsDlg.m_bXZVis )
		{
			m_pXZWnd->ShowWindow( SW_HIDE );
		}
		else
		{
			m_pXZWnd->ShowWindow( SW_SHOW );
		}

		g_PrefsDlg.m_bXZVis ^= 1;
		g_PrefsDlg.SavePrefs();
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnToggleviewYz()
{
	if( m_pYZWnd && m_pYZWnd->GetSafeHwnd() )
	{
		g_PrefsDlg.m_bYZVis = m_pYZWnd->IsWindowVisible();
		if( g_PrefsDlg.m_bYZVis )
		{
			m_pYZWnd->ShowWindow( SW_HIDE );
		}
		else
		{
			m_pYZWnd->ShowWindow( SW_SHOW );
		}

		g_PrefsDlg.m_bYZVis ^= 1;
		g_PrefsDlg.SavePrefs();
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */

void CMainFrame::OnToggleToolbar()
{
	//ShowControlBar(&m_wndToolBar, !m_wndToolBar.IsWindowVisible(), false);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnColorsBrush()
{
	DoColor( COLOR_BRUSHES );
	Sys_UpdateWindows( W_ALL );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnColorsClipper()
{
	DoColor( COLOR_CLIPPER );
	Sys_UpdateWindows( W_ALL );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnColorsGridtext()
{
	DoColor( COLOR_GRIDTEXT );
	Sys_UpdateWindows( W_ALL );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnColorsSelectedbrush()
{
	DoColor( COLOR_SELBRUSHES );
	Sys_UpdateWindows( W_ALL );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnColorsGridblock()
{
	DoColor( COLOR_GRIDBLOCK );
	Sys_UpdateWindows( W_ALL );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnColorsViewname()
{
	DoColor( COLOR_VIEWNAME );
	Sys_UpdateWindows( W_ALL );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnColorSetoriginal()
{
	for( int i = 0; i < 3; i++ )
	{
		g_qeglobals.d_savedinfo.colors[COLOR_TEXTUREBACK][i] = 0.0f;
		g_qeglobals.d_savedinfo.colors[COLOR_GRIDBACK][i] = 1.0f;
		g_qeglobals.d_savedinfo.colors[COLOR_GRIDMINOR][i] = 0.75f;
		g_qeglobals.d_savedinfo.colors[COLOR_GRIDMAJOR][i] = 0.5f;
		g_qeglobals.d_savedinfo.colors[COLOR_CAMERABACK][i] = 0.25f;
	}

	g_qeglobals.d_savedinfo.colors[COLOR_GRIDBLOCK][0] = 0.0f;
	g_qeglobals.d_savedinfo.colors[COLOR_GRIDBLOCK][1] = 0.0f;
	g_qeglobals.d_savedinfo.colors[COLOR_GRIDBLOCK][2] = 1.0f;

	g_qeglobals.d_savedinfo.colors[COLOR_GRIDTEXT][0] = 0.0f;
	g_qeglobals.d_savedinfo.colors[COLOR_GRIDTEXT][1] = 0.0f;
	g_qeglobals.d_savedinfo.colors[COLOR_GRIDTEXT][2] = 0.0f;

	g_qeglobals.d_savedinfo.colors[COLOR_SELBRUSHES][0] = 1.0f;
	g_qeglobals.d_savedinfo.colors[COLOR_SELBRUSHES][1] = 0.0f;
	g_qeglobals.d_savedinfo.colors[COLOR_SELBRUSHES][2] = 0.0f;

	g_qeglobals.d_savedinfo.colors[COLOR_CLIPPER][0] = 0.0f;
	g_qeglobals.d_savedinfo.colors[COLOR_CLIPPER][1] = 0.0f;
	g_qeglobals.d_savedinfo.colors[COLOR_CLIPPER][2] = 1.0f;

	g_qeglobals.d_savedinfo.colors[COLOR_BRUSHES][0] = 0.0f;
	g_qeglobals.d_savedinfo.colors[COLOR_BRUSHES][1] = 0.0f;
	g_qeglobals.d_savedinfo.colors[COLOR_BRUSHES][2] = 0.0f;

	g_qeglobals.d_savedinfo.colors[COLOR_VIEWNAME][0] = 0.5f;
	g_qeglobals.d_savedinfo.colors[COLOR_VIEWNAME][1] = 0.0f;
	g_qeglobals.d_savedinfo.colors[COLOR_VIEWNAME][2] = 0.75f;

	g_qeglobals.d_savedinfo.colors[COLOR_PRECISION_CROSSHAIR][0] = 1.0;
	g_qeglobals.d_savedinfo.colors[COLOR_PRECISION_CROSSHAIR][1] = 0.0;
	g_qeglobals.d_savedinfo.colors[COLOR_PRECISION_CROSSHAIR][2] = 1.0;

	Sys_UpdateWindows( W_ALL );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnColorSetqer()
{
	for( int i = 0; i < 3; i++ )
	{
		g_qeglobals.d_savedinfo.colors[COLOR_TEXTUREBACK][i] = 0.0f;
		g_qeglobals.d_savedinfo.colors[COLOR_GRIDBACK][i] = 1.0f;
		g_qeglobals.d_savedinfo.colors[COLOR_GRIDMINOR][i] = 1.0f;
		g_qeglobals.d_savedinfo.colors[COLOR_GRIDMAJOR][i] = 0.5f;
		g_qeglobals.d_savedinfo.colors[COLOR_CAMERABACK][i] = 0.25f;
	}

	g_qeglobals.d_savedinfo.colors[COLOR_GRIDBLOCK][0] = 0.0f;
	g_qeglobals.d_savedinfo.colors[COLOR_GRIDBLOCK][1] = 0.0f;
	g_qeglobals.d_savedinfo.colors[COLOR_GRIDBLOCK][2] = 1.0f;

	g_qeglobals.d_savedinfo.colors[COLOR_GRIDTEXT][0] = 0.0f;
	g_qeglobals.d_savedinfo.colors[COLOR_GRIDTEXT][1] = 0.0f;
	g_qeglobals.d_savedinfo.colors[COLOR_GRIDTEXT][2] = 0.0f;

	g_qeglobals.d_savedinfo.colors[COLOR_SELBRUSHES][0] = 1.0f;
	g_qeglobals.d_savedinfo.colors[COLOR_SELBRUSHES][1] = 0.0f;
	g_qeglobals.d_savedinfo.colors[COLOR_SELBRUSHES][2] = 0.0f;

	g_qeglobals.d_savedinfo.colors[COLOR_CLIPPER][0] = 0.0f;
	g_qeglobals.d_savedinfo.colors[COLOR_CLIPPER][1] = 0.0f;
	g_qeglobals.d_savedinfo.colors[COLOR_CLIPPER][2] = 1.0f;

	g_qeglobals.d_savedinfo.colors[COLOR_BRUSHES][0] = 0.0f;
	g_qeglobals.d_savedinfo.colors[COLOR_BRUSHES][1] = 0.0f;
	g_qeglobals.d_savedinfo.colors[COLOR_BRUSHES][2] = 0.0f;

	g_qeglobals.d_savedinfo.colors[COLOR_VIEWNAME][0] = 0.5f;
	g_qeglobals.d_savedinfo.colors[COLOR_VIEWNAME][1] = 0.0f;
	g_qeglobals.d_savedinfo.colors[COLOR_VIEWNAME][2] = 0.75f;

	g_qeglobals.d_savedinfo.colors[COLOR_PRECISION_CROSSHAIR][0] = 1.0;
	g_qeglobals.d_savedinfo.colors[COLOR_PRECISION_CROSSHAIR][1] = 0.0;
	g_qeglobals.d_savedinfo.colors[COLOR_PRECISION_CROSSHAIR][2] = 1.0;

	Sys_UpdateWindows( W_ALL );
}

//FIXME: these just need to be read from a def file
void CMainFrame::OnColorSetSuperMal()
{
	OnColorSetqer();
	g_qeglobals.d_savedinfo.colors[COLOR_TEXTUREBACK][0] = 0.0f;
	g_qeglobals.d_savedinfo.colors[COLOR_TEXTUREBACK][1] = 0.0f;
	g_qeglobals.d_savedinfo.colors[COLOR_TEXTUREBACK][2] = 0.0f;
	g_qeglobals.d_savedinfo.colors[COLOR_GRIDBACK][0] = 0.35f;
	g_qeglobals.d_savedinfo.colors[COLOR_GRIDBACK][1] = 0.35f;
	g_qeglobals.d_savedinfo.colors[COLOR_GRIDBACK][2] = 0.35f;
	g_qeglobals.d_savedinfo.colors[COLOR_GRIDMAJOR][0] = 0.5f;
	g_qeglobals.d_savedinfo.colors[COLOR_GRIDMAJOR][1] = 0.5f;
	g_qeglobals.d_savedinfo.colors[COLOR_GRIDMAJOR][2] = 0.5f;
	g_qeglobals.d_savedinfo.colors[COLOR_GRIDMINOR][0] = 0.39f;
	g_qeglobals.d_savedinfo.colors[COLOR_GRIDMINOR][1] = 0.39f;
	g_qeglobals.d_savedinfo.colors[COLOR_GRIDMINOR][2] = 0.39f;
	g_qeglobals.d_savedinfo.colors[COLOR_GRIDTEXT][0] = 0.0f;
	g_qeglobals.d_savedinfo.colors[COLOR_GRIDTEXT][1] = 0.0f;
	g_qeglobals.d_savedinfo.colors[COLOR_GRIDTEXT][2] = 0.0f;
	g_qeglobals.d_savedinfo.colors[COLOR_BRUSHES][0] = 0.0f;
	g_qeglobals.d_savedinfo.colors[COLOR_BRUSHES][1] = 0.0f;
	g_qeglobals.d_savedinfo.colors[COLOR_BRUSHES][2] = 0.0f;
	g_qeglobals.d_savedinfo.colors[COLOR_SELBRUSHES][0] = 1.0f;
	g_qeglobals.d_savedinfo.colors[COLOR_SELBRUSHES][1] = 0.90f;
	g_qeglobals.d_savedinfo.colors[COLOR_SELBRUSHES][2] = 0.90f;
	g_qeglobals.d_savedinfo.colors[COLOR_VIEWNAME][0] = 0.5f;
	g_qeglobals.d_savedinfo.colors[COLOR_VIEWNAME][1] = 0.0f;
	g_qeglobals.d_savedinfo.colors[COLOR_VIEWNAME][2] = 0.74f;


	Sys_UpdateWindows( W_ALL );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnColorSetblack()
{
	for( int i = 0; i < 3; i++ )
	{
		g_qeglobals.d_savedinfo.colors[COLOR_TEXTUREBACK][i] = 0.0f;
		g_qeglobals.d_savedinfo.colors[COLOR_GRIDBACK][i] = 0.0f;
		g_qeglobals.d_savedinfo.colors[COLOR_GRIDMINOR][i] = 0.0f;
		g_qeglobals.d_savedinfo.colors[COLOR_CAMERABACK][i] = 0.25f;
	}

	g_qeglobals.d_savedinfo.colors[COLOR_GRIDMAJOR][0] = 0.3f;
	g_qeglobals.d_savedinfo.colors[COLOR_GRIDMAJOR][1] = 0.5f;
	g_qeglobals.d_savedinfo.colors[COLOR_GRIDMAJOR][2] = 0.5f;

	g_qeglobals.d_savedinfo.colors[COLOR_GRIDBLOCK][0] = 0.0f;
	g_qeglobals.d_savedinfo.colors[COLOR_GRIDBLOCK][1] = 0.0f;
	g_qeglobals.d_savedinfo.colors[COLOR_GRIDBLOCK][2] = 1.0f;

	g_qeglobals.d_savedinfo.colors[COLOR_GRIDTEXT][0] = 1.0f;
	g_qeglobals.d_savedinfo.colors[COLOR_GRIDTEXT][1] = 1.0f;
	g_qeglobals.d_savedinfo.colors[COLOR_GRIDTEXT][2] = 1.0f;

	g_qeglobals.d_savedinfo.colors[COLOR_SELBRUSHES][0] = 1.0f;
	g_qeglobals.d_savedinfo.colors[COLOR_SELBRUSHES][1] = 0.0f;
	g_qeglobals.d_savedinfo.colors[COLOR_SELBRUSHES][2] = 0.0f;

	g_qeglobals.d_savedinfo.colors[COLOR_CLIPPER][0] = 0.0f;
	g_qeglobals.d_savedinfo.colors[COLOR_CLIPPER][1] = 0.0f;
	g_qeglobals.d_savedinfo.colors[COLOR_CLIPPER][2] = 1.0f;

	g_qeglobals.d_savedinfo.colors[COLOR_BRUSHES][0] = 1.0f;
	g_qeglobals.d_savedinfo.colors[COLOR_BRUSHES][1] = 1.0f;
	g_qeglobals.d_savedinfo.colors[COLOR_BRUSHES][2] = 1.0f;

	g_qeglobals.d_savedinfo.colors[COLOR_VIEWNAME][0] = 0.7f;
	g_qeglobals.d_savedinfo.colors[COLOR_VIEWNAME][1] = 0.7f;
	g_qeglobals.d_savedinfo.colors[COLOR_VIEWNAME][2] = 0.0f;

	g_qeglobals.d_savedinfo.colors[COLOR_PRECISION_CROSSHAIR][0] = 1.0;
	g_qeglobals.d_savedinfo.colors[COLOR_PRECISION_CROSSHAIR][1] = 0.0;
	g_qeglobals.d_savedinfo.colors[COLOR_PRECISION_CROSSHAIR][2] = 1.0;

	Sys_UpdateWindows( W_ALL );
}

void CMainFrame::OnColorSetMax()
{
	for( int i = 0 ; i < 3 ; i++ )
	{
		g_qeglobals.d_savedinfo.colors[COLOR_TEXTUREBACK][i] = 0.25f;
		g_qeglobals.d_savedinfo.colors[COLOR_GRIDBACK][i] = 0.77f;
		g_qeglobals.d_savedinfo.colors[COLOR_GRIDMINOR][i] = 0.83f;
		g_qeglobals.d_savedinfo.colors[COLOR_GRIDMAJOR][i] = 0.89f;
		g_qeglobals.d_savedinfo.colors[COLOR_CAMERABACK][i] = 0.25f;
	}

	g_qeglobals.d_savedinfo.colors[COLOR_GRIDBLOCK][0] = 1.0f;
	g_qeglobals.d_savedinfo.colors[COLOR_GRIDBLOCK][1] = 1.0f;
	g_qeglobals.d_savedinfo.colors[COLOR_GRIDBLOCK][2] = 1.0f;
	g_qeglobals.d_savedinfo.colors[COLOR_GRIDTEXT][0] = 0.0f;
	g_qeglobals.d_savedinfo.colors[COLOR_GRIDTEXT][1] = 0.0f;
	g_qeglobals.d_savedinfo.colors[COLOR_GRIDTEXT][2] = 0.0f;
	g_qeglobals.d_savedinfo.colors[COLOR_SELBRUSHES][0] = 1.0f;
	g_qeglobals.d_savedinfo.colors[COLOR_SELBRUSHES][1] = 0.0f;
	g_qeglobals.d_savedinfo.colors[COLOR_SELBRUSHES][2] = 0.0f;
	g_qeglobals.d_savedinfo.colors[COLOR_CLIPPER][0] = 0.0f;
	g_qeglobals.d_savedinfo.colors[COLOR_CLIPPER][1] = 0.0f;
	g_qeglobals.d_savedinfo.colors[COLOR_CLIPPER][2] = 1.0f;
	g_qeglobals.d_savedinfo.colors[COLOR_BRUSHES][0] = 0.0f;
	g_qeglobals.d_savedinfo.colors[COLOR_BRUSHES][1] = 0.0f;
	g_qeglobals.d_savedinfo.colors[COLOR_BRUSHES][2] = 0.0f;
	g_qeglobals.d_savedinfo.colors[COLOR_VIEWNAME][0] = 0.5f;
	g_qeglobals.d_savedinfo.colors[COLOR_VIEWNAME][1] = 0.0f;
	g_qeglobals.d_savedinfo.colors[COLOR_VIEWNAME][2] = 0.75f;

	//g_qeglobals.d_savedinfo.colors[COLOR_CAMERABACK][0] = 0.0f;
	//g_qeglobals.d_savedinfo.colors[COLOR_CAMERABACK][1] = 1.0f;
	//g_qeglobals.d_savedinfo.colors[COLOR_CAMERABACK][2] = 1.0f;

	g_qeglobals.d_savedinfo.colors[COLOR_PRECISION_CROSSHAIR][0] = 1.0f;
	g_qeglobals.d_savedinfo.colors[COLOR_PRECISION_CROSSHAIR][1] = 0.0f;
	g_qeglobals.d_savedinfo.colors[COLOR_PRECISION_CROSSHAIR][2] = 1.0f;

	Sys_UpdateWindows( W_ALL );

}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnSnaptogrid()
{
	g_PrefsDlg.m_bNoClamp ^= 1;
	g_PrefsDlg.SavePrefs();

	CMenu*	pMenu = GetMenu();
	if( pMenu )
	{
		pMenu->CheckMenuItem( ID_SNAPTOGRID, MF_BYCOMMAND | ( !g_PrefsDlg.m_bNoClamp ) ? MF_CHECKED : MF_UNCHECKED );
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnSelectScale()
{
	// if (ActiveXY()) ActiveXY()->UndoCopy();
	Undo_Start( "scale" );
	Undo_AddBrushList( &selected_brushes );

	CScaleDialog	dlg;
	if( dlg.DoModal() == IDOK )
	{
		if( dlg.m_fX > 0 && dlg.m_fY > 0 && dlg.m_fZ > 0 )
		{
			Select_Scale( dlg.m_fX, dlg.m_fY, dlg.m_fZ );
			Sys_UpdateWindows( W_ALL );
		}
		else
		{
			common->Printf( "Warning.. Tried to scale by a zero value." );
		}
	}

	Undo_EndBrushList( &selected_brushes );
	Undo_End();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnSelectMouserotate()
{
	if( ActiveXY() )
	{
		if( ActiveXY()->ClipMode() )
		{
			OnViewClipper();
		}

		if( ActiveXY()->RotateMode() )
		{
			ActiveXY()->SetRotateMode( false );
			SetToolbarButtonCheckState( m_wndToolBar, ID_SELECT_MOUSEROTATE, FALSE );
			Map_BuildBrushData();
		}
		else
		{
			// may not work if no brush selected, see return value
			if( ActiveXY()->SetRotateMode( true ) )
			{
				g_qeglobals.rotateAxis = 0;
				if( ActiveXY()->GetViewType() == ViewType::XY )
				{
					g_qeglobals.rotateAxis = 2;
				}
				else if( ActiveXY()->GetViewType() == ViewType::XZ )
				{
					g_qeglobals.rotateAxis = 1;
				}
				SetToolbarButtonCheckState( m_wndToolBar, ID_SELECT_MOUSEROTATE, TRUE );
			}
			else  	// if MFC called, we need to set back to FALSE ourselves
			{
				SetToolbarButtonCheckState( m_wndToolBar, ID_SELECT_MOUSEROTATE, FALSE );
			}
		}
	}
	Sys_UpdateWindows( W_CAMERA | W_XY );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnEditCopybrush()
{
	if( ActiveXY() )
	{
		ActiveXY()->Copy();
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnEditPastebrush()
{
	if( ActiveXY() )
	{
		ActiveXY()->Paste();
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnEditUndo()
{
	// if (ActiveXY()) ActiveXY()->Undo();
	Undo_Undo();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnEditRedo()
{
	Undo_Redo();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnUpdateEditUndo( CCmdUI* pCmdUI )
{
	/*
	 * BOOL bEnable = false; if (ActiveXY()) bEnable = ActiveXY()->UndoAvailable();
	 * pCmdUI->Enable(bEnable);
	 */
	pCmdUI->Enable( Undo_UndoAvailable() );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnUpdateEditRedo( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( Undo_RedoAvailable() );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnSelectionTextureDec()
{
	g_qeglobals.d_savedinfo.m_nTextureTweak -= 1.0f;
	if( g_qeglobals.d_savedinfo.m_nTextureTweak == 0.0f )
	{
		g_qeglobals.d_savedinfo.m_nTextureTweak -= 1.0f;
	}

	SetTexValStatus();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnSelectionTextureFit()
{
	Select_FitTexture( 1.0f, 1.0f );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnSelectionTextureInc()
{
	g_qeglobals.d_savedinfo.m_nTextureTweak += 1.0f;
	if( g_qeglobals.d_savedinfo.m_nTextureTweak == 0.0f )
	{
		g_qeglobals.d_savedinfo.m_nTextureTweak += 1.0f;
	}

	SetTexValStatus();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnSelectionTextureRotateclock()
{
	Select_RotateTexture( abs( g_PrefsDlg.m_nRotation ) );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnSelectionTextureRotatecounter()
{
	Select_RotateTexture( -abs( g_PrefsDlg.m_nRotation ) );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnSelectionTextureScaledown()
{
	Select_ScaleTexture( 0, -g_qeglobals.d_savedinfo.m_nTextureTweak );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnSelectionTextureScaleup()
{
	Select_ScaleTexture( 0, g_qeglobals.d_savedinfo.m_nTextureTweak );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnSelectionTextureScaleLeft()
{
	Select_ScaleTexture( g_qeglobals.d_savedinfo.m_nTextureTweak, 0 );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnSelectionTextureScaleRight()
{
	Select_ScaleTexture( g_qeglobals.d_savedinfo.m_nTextureTweak, 0 );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnSelectionTextureShiftdown()
{
	Select_ShiftTexture( 0, -g_qeglobals.d_savedinfo.m_nTextureTweak, true );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnSelectionTextureShiftleft()
{
	Select_ShiftTexture( -g_qeglobals.d_savedinfo.m_nTextureTweak, 0, true );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnSelectionTextureShiftright()
{
	Select_ShiftTexture( g_qeglobals.d_savedinfo.m_nTextureTweak, 0, true );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnSelectionTextureShiftup()
{
	Select_ShiftTexture( 0, g_qeglobals.d_savedinfo.m_nTextureTweak, true );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::SetGridChecks( int id )
{
	HMENU	hMenu = ::GetMenu( GetSafeHwnd() );
	CheckMenuItem( hMenu, ID_GRID_1, MF_BYCOMMAND | MF_UNCHECKED );
	CheckMenuItem( hMenu, ID_GRID_2, MF_BYCOMMAND | MF_UNCHECKED );
	CheckMenuItem( hMenu, ID_GRID_4, MF_BYCOMMAND | MF_UNCHECKED );
	CheckMenuItem( hMenu, ID_GRID_8, MF_BYCOMMAND | MF_UNCHECKED );
	CheckMenuItem( hMenu, ID_GRID_16, MF_BYCOMMAND | MF_UNCHECKED );
	CheckMenuItem( hMenu, ID_GRID_32, MF_BYCOMMAND | MF_UNCHECKED );
	CheckMenuItem( hMenu, ID_GRID_64, MF_BYCOMMAND | MF_UNCHECKED );
	CheckMenuItem( hMenu, ID_GRID_POINT5, MF_BYCOMMAND | MF_UNCHECKED );
	CheckMenuItem( hMenu, ID_GRID_POINT25, MF_BYCOMMAND | MF_UNCHECKED );
	CheckMenuItem( hMenu, ID_GRID_POINT125, MF_BYCOMMAND | MF_UNCHECKED );
	CheckMenuItem( hMenu, ID_GRID_POINT0625, MF_BYCOMMAND | MF_UNCHECKED );
	CheckMenuItem( hMenu, id, MF_BYCOMMAND | MF_CHECKED );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnGridNext()
{
	if( g_qeglobals.d_gridsize >= MAX_GRID )
	{
		return;
	}

	g_qeglobals.d_gridsize *= 2.0f;

	float	minGrid = MIN_GRID;
	int		id = ID_GRID_START;

	while( minGrid < g_qeglobals.d_gridsize && id < ID_GRID_END )
	{
		minGrid *= 2.0f;
		id++;
	}

	UpdateGrid();

	SetGridChecks( id );
	SetGridStatus();
	Sys_UpdateWindows( W_XY | W_Z | W_CAMERA );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnGridPrev()
{
	if( g_qeglobals.d_gridsize <= MIN_GRID )
	{
		return;
	}

	g_qeglobals.d_gridsize /= 2;

	float	maxGrid = MAX_GRID;
	int		id = ID_GRID_END;

	while( maxGrid > g_qeglobals.d_gridsize && id > ID_GRID_START )
	{
		maxGrid /= 2.0f;
		id--;
	}

	UpdateGrid();

	SetGridChecks( id );
	SetGridStatus();
	Sys_UpdateWindows( W_XY | W_Z | W_CAMERA );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::SetGridStatus()
{
	CString strStatus;
	char	c1;
	char	c2;
	c1 = ( g_PrefsDlg.m_bTextureLock ) ? 'M' : ' ';
	c2 = ( g_PrefsDlg.m_bRotateLock ) ? 'R' : ' ';
	strStatus.Format
	(
		"G:%1.2f T:%1.2f R:%i C:%i L:%c%c",
		g_qeglobals.d_gridsize,
		g_qeglobals.d_savedinfo.m_nTextureTweak,
		g_PrefsDlg.m_nRotation,
		g_PrefsDlg.m_nCubicScale,
		c1,
		c2
	);
	SetStatusText( 4, strStatus );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::SetTexValStatus()
{
	//
	// CString strStatus; strStatus.Format("T: %i C: %i", g_nTextureTweak,
	// g_nCubicScale); SetStatusText(5, strStatus.GetBuffer(0));
	//
	SetGridStatus();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnTextureReplaceall()
{
	CFindTextureDlg::show();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnScalelockx()
{
	if( g_nScaleHow & SCALE_X )
	{
		SetToolbarButtonCheckState( m_wndToolBar, ID_SCALELOCKX, FALSE );
	}
	else
	{
		SetToolbarButtonCheckState( m_wndToolBar, ID_SCALELOCKX, TRUE );
	}

	g_nScaleHow ^= SCALE_X;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnScalelocky()
{
	if( g_nScaleHow & SCALE_Y )
	{
		SetToolbarButtonCheckState( m_wndToolBar, ID_SCALELOCKY, FALSE );
	}
	else
	{
		SetToolbarButtonCheckState( m_wndToolBar, ID_SCALELOCKY, TRUE );
	}

	g_nScaleHow ^= SCALE_Y;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnScalelockz()
{
	if( g_nScaleHow & SCALE_Z )
	{
		SetToolbarButtonCheckState( m_wndToolBar, ID_SCALELOCKZ, FALSE );
	}
	else
	{
		SetToolbarButtonCheckState( m_wndToolBar, ID_SCALELOCKZ, TRUE );
	}

	g_nScaleHow ^= SCALE_Z;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnSelectMousescale()
{
	if( ActiveXY() )
	{
		if( ActiveXY()->ClipMode() )
		{
			OnViewClipper();
		}

		if( ActiveXY()->RotateMode() )
		{
			// SetRotateMode(false) always works
			ActiveXY()->SetRotateMode( false );
			SetToolbarButtonCheckState( m_wndToolBar, ID_SELECT_MOUSESCALE, FALSE );
		}

		if( ActiveXY()->ScaleMode() )
		{
			ActiveXY()->SetScaleMode( false );
			SetToolbarButtonCheckState( m_wndToolBar, ID_SELECT_MOUSESCALE, FALSE );
		}
		else
		{
			ActiveXY()->SetScaleMode( true );
			SetToolbarButtonCheckState( m_wndToolBar, ID_SELECT_MOUSESCALE, TRUE );
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnFileImport()
{
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnUpdateFileImport( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( FALSE );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnViewCubein()
{
	g_PrefsDlg.m_nCubicScale--;
	if( g_PrefsDlg.m_nCubicScale < 1 )
	{
		g_PrefsDlg.m_nCubicScale = 1;
	}

	g_PrefsDlg.SavePrefs();
	Sys_UpdateWindows( W_CAMERA );
	SetTexValStatus();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnViewCubeout()
{
	g_PrefsDlg.m_nCubicScale++;
	if( g_PrefsDlg.m_nCubicScale > 99 )
	{
		g_PrefsDlg.m_nCubicScale = 99;
	}

	g_PrefsDlg.SavePrefs();
	Sys_UpdateWindows( W_CAMERA );
	SetTexValStatus();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnViewCubicclipping()
{
	g_PrefsDlg.m_bCubicClipping ^= 1;

	CMenu*	pMenu = GetMenu();
	if( pMenu )
	{
		pMenu->CheckMenuItem
		(
			ID_VIEW_CUBICCLIPPING,
			MF_BYCOMMAND | ( g_PrefsDlg.m_bCubicClipping ) ? MF_CHECKED : MF_UNCHECKED
		);
	}

	SetToolbarButtonCheckState( m_wndToolBar, ID_VIEW_CUBICCLIPPING, ( g_PrefsDlg.m_bCubicClipping ) ? TRUE : FALSE );
	g_PrefsDlg.SavePrefs();
	Map_BuildBrushData();
	Sys_UpdateWindows( W_CAMERA );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnFileSaveregion()
{
	SaveAsDialog( true );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnUpdateFileSaveregion( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( static_cast<BOOL>( region_active ) );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnSelectionMovedown()
{
	Undo_Start( "move up" );
	Undo_AddBrushList( &selected_brushes );

	idVec3	vAmt;
	vAmt[0] = vAmt[1] = 0.0f;
	vAmt[2] = -g_qeglobals.d_gridsize;
	Select_Move( vAmt );
	Sys_UpdateWindows( W_CAMERA | W_XY | W_Z );

	Undo_EndBrushList( &selected_brushes );
	Undo_End();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnSelectionMoveup()
{
	idVec3	vAmt;
	vAmt[0] = vAmt[1] = 0.0f;
	vAmt[2] = g_qeglobals.d_gridsize;
	Select_Move( vAmt );
	Sys_UpdateWindows( W_CAMERA | W_XY | W_Z );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnSelectionPrint()
{
	for( idEditorBrush* b = selected_brushes.next; b != &selected_brushes; b = b->next )
	{
		Brush_Print( b );
	}
}

bool	g_bTABDown = false;
bool	g_bOriginalFlag;

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnSelectionTogglesizepaint()
{
	if( ::GetAsyncKeyState( 'Q' ) )
	{
		if( !g_bTABDown )
		{
			g_bTABDown = true;
			g_bOriginalFlag = ( g_PrefsDlg.m_bSizePaint != FALSE );
			g_PrefsDlg.m_bSizePaint = !g_bOriginalFlag;
			Sys_UpdateWindows( W_XY );
			return;
		}
	}
	else
	{
		g_bTABDown = false;
		g_PrefsDlg.m_bSizePaint = g_bOriginalFlag;
		Sys_UpdateWindows( W_XY );
		return;
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnBrushMakecone()
{
	Undo_Start( "make cone" );
	Undo_AddBrushList( &selected_brushes );
	DoSides( true );
	Undo_EndBrushList( &selected_brushes );
	Undo_End();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnTexturesLoad()
{
	BROWSEINFO	bi;
	CString		strPath;
	char*		p = strPath.GetBuffer( MAX_PATH + 1 );
	bi.hwndOwner = GetSafeHwnd();
	bi.pidlRoot = NULL;
	bi.pszDisplayName = p;
	bi.lpszTitle = "Load textures from path";
	bi.ulFlags = 0;
	bi.lpfn = NULL;
	bi.lParam = NULL;
	bi.iImage = 0;

	LPITEMIDLIST	pidlBrowse;
	pidlBrowse = SHBrowseForFolder( &bi );
	if( pidlBrowse )
	{
		SHGetPathFromIDList( pidlBrowse, p );
		strPath.ReleaseBuffer();
		AddSlash( strPath );
		//FIXME: idMaterial
		//Texture_ShowDirectory(strPath.GetBuffer(0));
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnToggleRotatelock()
{
	g_qeglobals.flatRotation = false;
	g_qeglobals.rotateAxis++;
	if( g_qeglobals.rotateAxis > 2 )
	{
		g_qeglobals.rotateAxis = 0;
	}
	Select_InitializeRotation();
	Sys_UpdateWindows( W_CAMERA | W_XY );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnCurveBevel()
{
	// Curve_MakeCurvedBrush (false, false, false, false, false, true, true);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnCurveCylinder()
{
	// Curve_MakeCurvedBrush (false, false, false, true, true, true, true);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnCurveEighthsphere()
{
	// Curve_MakeCurvedBrush (false, true, false, true, true, false, false);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnCurveEndcap()
{
	// Curve_MakeCurvedBrush (false, false, false, false, true, true, true);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnCurveHemisphere()
{
	// Curve_MakeCurvedBrush (false, true, false, true, true, true, true);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnCurveInvertcurve()
{
	// Curve_Invert ();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnCurveQuarter()
{
	// Curve_MakeCurvedBrush (false, true, false, true, true, true, false);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnCurveSphere()
{
	// Curve_MakeCurvedBrush (false, true, true, true, true, true, true);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnFileImportmap()
{
	CFileDialog dlgFile( TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "Map files (*.map)|*.map||", this );
	if( dlgFile.DoModal() == IDOK )
	{
		Map_ImportFile( dlgFile.GetPathName().GetBuffer( 0 ) );
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnFileExportmap()
{
	CFileDialog dlgFile( FALSE, "map", NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "Map files (*.map)|*.map||", this );
	if( dlgFile.DoModal() == IDOK )
	{
		Map_SaveSelected( dlgFile.GetPathName().GetBuffer( 0 ) );
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnViewShowcurves()
{
	if( ( g_qeglobals.d_savedinfo.exclude ^= EXCLUDE_CURVES ) & EXCLUDE_CURVES )
	{
		CheckMenuItem( ::GetMenu( GetSafeHwnd() ), ID_VIEW_SHOWCURVES, MF_BYCOMMAND | MF_UNCHECKED );
	}
	else
	{
		CheckMenuItem( ::GetMenu( GetSafeHwnd() ), ID_VIEW_SHOWCURVES, MF_BYCOMMAND | MF_CHECKED );
	}

	Sys_UpdateWindows( W_XY | W_CAMERA );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnSelectionSelectNudgedown()
{
	NudgeSelection( 3, g_qeglobals.d_savedinfo.m_nTextureTweak );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnSelectionSelectNudgeleft()
{
	NudgeSelection( 0, g_qeglobals.d_savedinfo.m_nTextureTweak );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnSelectionSelectNudgeright()
{
	NudgeSelection( 2, g_qeglobals.d_savedinfo.m_nTextureTweak );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnSelectionSelectNudgeup()
{
	NudgeSelection( 1, g_qeglobals.d_savedinfo.m_nTextureTweak );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::NudgeSelection( int nDirection, float fAmount )
{
	if( ActiveXY()->RotateMode() )
	{
		int nAxis = 0;
		if( ActiveXY()->GetViewType() == ViewType::XY )
		{
			nAxis = 2;
		}
		else if( g_pParentWnd->ActiveXY()->GetViewType() == ViewType::XZ )
		{
			nAxis = 1;
			fAmount = -fAmount;
		}

		if( nDirection == 2 || nDirection == 3 )
		{
			fAmount = -fAmount;
		}

		float	fDeg = -fAmount;

		g_pParentWnd->ActiveXY()->Rotation()[nAxis] += fAmount;

		CString strStatus;
		strStatus.Format
		(
			"Rotation x:: %.1f  y:: %.1f  z:: %.1f",
			g_pParentWnd->ActiveXY()->Rotation()[0],
			g_pParentWnd->ActiveXY()->Rotation()[1],
			g_pParentWnd->ActiveXY()->Rotation()[2]
		);
		g_pParentWnd->SetStatusText( 2, strStatus );
		Select_RotateAxis( nAxis, fDeg, false, true );
		Sys_UpdateWindows( W_ALL );
	}
	else if( ActiveXY()->ScaleMode() )
	{
		if( nDirection == 0 || nDirection == 3 )
		{
			fAmount = -fAmount;
		}

		idVec3	v;
		v[0] = v[1] = v[2] = 1.0f;
		if( fAmount > 0 )
		{
			v[0] = 1.1f;
			v[1] = 1.1f;
			v[2] = 1.1f;
		}
		else
		{
			v[0] = 0.9f;
			v[1] = 0.9f;
			v[2] = 0.9f;
		}

		Select_Scale
		(
			( g_nScaleHow & SCALE_X ) ? v[0] : 1.0f,
			( g_nScaleHow & SCALE_Y ) ? v[1] : 1.0f,
			( g_nScaleHow & SCALE_Z ) ? v[2] : 1.0f
		);
		Sys_UpdateWindows( W_ALL );
	}
	else
	{
		// 0 - left, 1 - up, 2 - right, 3 - down
		int nDim;
		if( nDirection == 0 )
		{
			nDim = ActiveXY()->GetViewType() == ViewType::YZ ? 1 : 0;
			fAmount = -fAmount;
		}
		else if( nDirection == 1 )
		{
			nDim = ActiveXY()->GetViewType() == ViewType::XY ? 1 : 2;
		}
		else if( nDirection == 2 )
		{
			nDim = ActiveXY()->GetViewType() == ViewType::YZ ? 1 : 0;
		}
		else
		{
			nDim = ActiveXY()->GetViewType() == ViewType::XY ? 1 : 2;
			fAmount = -fAmount;
		}

		Nudge( nDim, fAmount );
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::Nudge( int nDim, float fNudge )
{
	idVec3	vMove;
	vMove[0] = vMove[1] = vMove[2] = 0;
	vMove[nDim] = fNudge;
	Select_Move( vMove, true );
	Sys_UpdateWindows( W_ALL );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnTexturesLoadlist()
{
	CDialogTextures dlg;
	dlg.DoModal();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnSelectByBoundingBrush()
{
	g_PrefsDlg.m_selectByBoundingBrush ^= 1;
	SetToolbarButtonCheckState( m_wndToolBar, ID_SELECT_BYBOUNDINGBRUSH, ( g_PrefsDlg.m_selectByBoundingBrush ) ? TRUE : FALSE );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnSelectBrushesOnly()
{
	g_PrefsDlg.m_selectOnlyBrushes ^= 1;
	SetToolbarButtonCheckState( m_wndToolBar, ID_SELECT_BRUSHESONLY, ( g_PrefsDlg.m_selectOnlyBrushes ) ? TRUE : FALSE );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnDynamicLighting()
{
	CCamWnd* pCam = new CCamWnd();
	CRect	rect( 100, 100, 300, 300 );
	pCam->Create( IDD_DIALOG_CAMERA, this );
	pCam->ShowWindow( SW_SHOW );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnCurveSimplepatchmesh()
{
	Undo_Start( "make simpe patch mesh" );
	Undo_AddBrushList( &selected_brushes );

	CPatchDensityDlg	dlg;
	dlg.DoModal();

	Undo_EndBrushList( &selected_brushes );
	Undo_End();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnPatchToggleBox()
{
	g_bPatchShowBounds ^= 1;
	SetToolbarButtonCheckState( m_wndToolBar, ID_PATCH_SHOWBOUNDINGBOX, ( g_bPatchShowBounds ) ? TRUE : FALSE );
	Sys_UpdateWindows( W_ALL );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnPatchWireframe()
{
	g_bPatchWireFrame ^= 1;
	SetToolbarButtonCheckState( m_wndToolBar, ID_PATCH_WIREFRAME, ( g_bPatchWireFrame ) ? TRUE : FALSE );
	Sys_UpdateWindows( W_ALL );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnCurvePatchcone()
{
	Undo_Start( "make curve cone" );
	Undo_AddBrushList( &selected_brushes );
	Patch_BrushToMesh( true );
	Sys_UpdateWindows( W_ALL );
	Undo_EndBrushList( &selected_brushes );
	Undo_End();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnCurvePatchtube()
{
	Undo_Start( "make curve cylinder" );
	Undo_AddBrushList( &selected_brushes );
	Patch_BrushToMesh( false );
	Sys_UpdateWindows( W_ALL );
	Undo_EndBrushList( &selected_brushes );
	Undo_End();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnPatchWeld()
{
	g_bPatchWeld ^= 1;
	SetToolbarButtonCheckState( m_wndToolBar, ID_PATCH_WELD, ( g_bPatchWeld ) ? TRUE : FALSE );
	Sys_UpdateWindows( W_ALL );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnCurvePatchbevel()
{
	Undo_Start( "make bevel" );
	Undo_AddBrushList( &selected_brushes );
	Patch_BrushToMesh( false, true, false );
	Sys_UpdateWindows( W_ALL );
	Undo_EndBrushList( &selected_brushes );
	Undo_End();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnCurvePatchendcap()
{
	Undo_Start( "make end cap" );
	Undo_AddBrushList( &selected_brushes );
	Patch_BrushToMesh( false, false, true );
	Sys_UpdateWindows( W_ALL );
	Undo_EndBrushList( &selected_brushes );
	Undo_End();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnCurvePatchinvertedbevel()
{
	// Patch_BrushToMesh(false, true, false, true); Sys_UpdateWindows (W_ALL);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnCurvePatchinvertedendcap()
{
	// Patch_BrushToMesh(false, false, true, true); Sys_UpdateWindows (W_ALL);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnPatchDrilldown()
{
	g_bPatchDrillDown ^= 1;
	SetToolbarButtonCheckState( m_wndToolBar, ID_PATCH_DRILLDOWN, ( g_bPatchDrillDown ) ? TRUE : FALSE );
	Sys_UpdateWindows( W_ALL );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnCurveInsertcolumn()
{
	Undo_Start( "insert colum" );
	Undo_AddBrushList( &selected_brushes );

	// Patch_AdjustSelectedRowCols(0, 2);
	Patch_AdjustSelected( true, true, true );
	Sys_UpdateWindows( W_ALL );
	Undo_EndBrushList( &selected_brushes );
	Undo_End();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnCurveInsertrow()
{
	Undo_Start( "insert row" );
	Undo_AddBrushList( &selected_brushes );

	// Patch_AdjustSelectedRowCols(2, 0);
	Patch_AdjustSelected( true, false, true );
	Sys_UpdateWindows( W_ALL );
	Undo_EndBrushList( &selected_brushes );
	Undo_End();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnCurveDeletecolumn()
{
	Undo_Start( "delete column" );
	Undo_AddBrushList( &selected_brushes );
	Patch_AdjustSelected( false, true, true );
	Sys_UpdateWindows( W_ALL );
	Undo_EndBrushList( &selected_brushes );
	Undo_End();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnCurveDeleterow()
{
	Undo_Start( "delete row" );
	Undo_AddBrushList( &selected_brushes );
	Patch_AdjustSelected( false, false, true );
	Sys_UpdateWindows( W_ALL );
	Undo_EndBrushList( &selected_brushes );
	Undo_End();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnCurveInsertAddcolumn()
{
	Undo_Start( "add (2) columns" );
	Undo_AddBrushList( &selected_brushes );
	Patch_AdjustSelected( true, true, true );
	Sys_UpdateWindows( W_ALL );
	Undo_EndBrushList( &selected_brushes );
	Undo_End();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnCurveInsertAddrow()
{
	Undo_Start( "add (2) rows" );
	Undo_AddBrushList( &selected_brushes );
	Patch_AdjustSelected( true, false, true );
	Sys_UpdateWindows( W_ALL );
	Undo_EndBrushList( &selected_brushes );
	Undo_End();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnCurveInsertInsertcolumn()
{
	Undo_Start( "insert (2) columns" );
	Undo_AddBrushList( &selected_brushes );
	Patch_AdjustSelected( true, true, false );
	Sys_UpdateWindows( W_ALL );
	Undo_EndBrushList( &selected_brushes );
	Undo_End();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnCurveInsertInsertrow()
{
	Undo_Start( "insert (2) rows" );
	Undo_AddBrushList( &selected_brushes );
	Patch_AdjustSelected( true, false, false );
	Sys_UpdateWindows( W_ALL );
	Undo_EndBrushList( &selected_brushes );
	Undo_End();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnCurveNegative()
{
	Patch_ToggleInverted();

	// Sys_UpdateWindows(W_ALL);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnCurveNegativeTextureX()
{
	Select_FlipTexture( false );

	// Sys_UpdateWindows(W_ALL);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnCurveNegativeTextureY()
{
	Select_FlipTexture( true );
	// Sys_UpdateWindows(W_ALL);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnCurveDeleteFirstcolumn()
{
	Undo_Start( "delete first (2) columns" );
	Undo_AddBrushList( &selected_brushes );
	Patch_AdjustSelected( false, true, true );
	Sys_UpdateWindows( W_ALL );
	Undo_EndBrushList( &selected_brushes );
	Undo_End();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnCurveDeleteFirstrow()
{
	Undo_Start( "delete first (2) rows" );
	Undo_AddBrushList( &selected_brushes );
	Patch_AdjustSelected( false, false, true );
	Sys_UpdateWindows( W_ALL );
	Undo_EndBrushList( &selected_brushes );
	Undo_End();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnCurveDeleteLastcolumn()
{
	Undo_Start( "delete last (2) columns" );
	Undo_AddBrushList( &selected_brushes );
	Patch_AdjustSelected( false, true, false );
	Sys_UpdateWindows( W_ALL );
	Undo_EndBrushList( &selected_brushes );
	Undo_End();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnCurveDeleteLastrow()
{
	Undo_Start( "delete last (2) rows" );
	Undo_AddBrushList( &selected_brushes );
	Patch_AdjustSelected( false, false, false );
	Sys_UpdateWindows( W_ALL );
	Undo_EndBrushList( &selected_brushes );
	Undo_End();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnPatchBend()
{
	Patch_BendToggle();
	SetToolbarButtonCheckState( m_wndToolBar, ID_PATCH_BEND, ( g_bPatchBendMode ) ? TRUE : FALSE );
	Sys_UpdateWindows( W_ALL );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnPatchInsdel()
{
	Patch_InsDelToggle();
	SetToolbarButtonCheckState( m_wndToolBar, ID_PATCH_INSDEL, ( g_bPatchInsertMode ) ? TRUE : FALSE );
	Sys_UpdateWindows( W_ALL );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnPatchEnter()
{
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
extern bool Sys_KeyDown( int key );
void CMainFrame::OnPatchTab()
{
	if( g_bPatchBendMode )
	{
		Patch_BendHandleTAB();
	}
	else if( g_bPatchInsertMode )
	{
		Patch_InsDelHandleTAB();
	}
	else if( g_bAxialMode )
	{
		int faceCount = g_ptrSelectedFaces.GetSize();
		if( faceCount > 0 )
		{
			face_t*	selFace = reinterpret_cast < face_t* >( g_ptrSelectedFaces.GetAt( 0 ) );
			int* ip = ( Sys_KeyDown( VK_SHIFT ) ) ? &g_axialAnchor : &g_axialDest;
			( *ip )++;
			if( *ip >= selFace->face_winding->GetNumPoints() )
			{
				*ip = 0;
			}
		}
		Sys_UpdateWindows( W_CAMERA );
	}
	else
	{
		//
		// check to see if the selected brush is part of a func group if it is, deselect
		// everything and reselect the next brush in the group
		//
		idEditorBrush*		b = selected_brushes.next;
		idEditorEntity*	e;
		if( b != &selected_brushes )
		{
			if( idStr::Icmp( b->owner->eclass->name, "worldspawn" ) != 0 )
			{
				e = b->owner;
				Select_Deselect();
				idEditorBrush* b2;
				for( b2 = e->brushes.onext; b2 != &e->brushes; b2 = b2->onext )
				{
					if( b == b2 )
					{
						b2 = b2->onext;
						break;
					}
				}

				if( b2 == &e->brushes )
				{
					b2 = b2->onext;
				}

				Select_Brush( b2, false );
				Sys_UpdateWindows( W_ALL );
			}
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::UpdatePatchToolbarButtons()
{
	SetToolbarButtonCheckState( m_wndToolBar, ID_PATCH_BEND, ( g_bPatchBendMode ) ? TRUE : FALSE );
	SetToolbarButtonCheckState( m_wndToolBar, ID_PATCH_INSDEL, ( g_bPatchInsertMode ) ? TRUE : FALSE );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnCurvePatchdensetube()
{
	Undo_Start( "dense cylinder" );
	Undo_AddBrushList( &selected_brushes );

	Patch_BrushToMesh( false );
	OnCurveInsertAddrow();
	OnCurveInsertInsertrow();
	Sys_UpdateWindows( W_ALL );

	Undo_EndBrushList( &selected_brushes );
	Undo_End();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnCurvePatchverydensetube()
{
	Undo_Start( "very dense cylinder" );
	Undo_AddBrushList( &selected_brushes );

	Patch_BrushToMesh( false );
	OnCurveInsertAddrow();
	OnCurveInsertInsertrow();
	OnCurveInsertAddrow();
	OnCurveInsertInsertrow();
	Sys_UpdateWindows( W_ALL );

	Undo_EndBrushList( &selected_brushes );
	Undo_End();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnCurveCap()
{
	Patch_CapCurrent();
	Sys_UpdateWindows( W_ALL );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnCurveCapInvertedbevel()
{
	Patch_CapCurrent( true );
	Sys_UpdateWindows( W_ALL );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnCurveCapInvertedendcap()
{
	Patch_CapCurrent( false, true );
	Sys_UpdateWindows( W_ALL );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnCurveRedisperseCols()
{
	Patch_DisperseColumns();
	Sys_UpdateWindows( W_ALL );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnCurveRedisperseRows()
{
	Patch_DisperseRows();
	Sys_UpdateWindows( W_ALL );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnPatchNaturalize()
{
	Patch_NaturalizeSelected();
	Sys_UpdateWindows( W_ALL );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnPatchNaturalizeAlt()
{
	Patch_NaturalizeSelected( false, false, true );
	Sys_UpdateWindows( W_ALL );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnSnapToGrid()
{
	Select_SnapToGrid();
	Sys_UpdateWindows( W_ALL );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnCurvePatchsquare()
{
	Undo_Start( "square cylinder" );
	Undo_AddBrushList( &selected_brushes );

	Patch_BrushToMesh( false, false, false, true );
	Sys_UpdateWindows( W_ALL );

	Undo_EndBrushList( &selected_brushes );
	Undo_End();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::CheckTextureScale( int id )
{
	CMenu*	pMenu = GetMenu();
	if( pMenu )
	{
		pMenu->CheckMenuItem( ID_TEXTURES_TEXTUREWINDOWSCALE_10, MF_BYCOMMAND | MF_UNCHECKED );
		pMenu->CheckMenuItem( ID_TEXTURES_TEXTUREWINDOWSCALE_25, MF_BYCOMMAND | MF_UNCHECKED );
		pMenu->CheckMenuItem( ID_TEXTURES_TEXTUREWINDOWSCALE_50, MF_BYCOMMAND | MF_UNCHECKED );
		pMenu->CheckMenuItem( ID_TEXTURES_TEXTUREWINDOWSCALE_100, MF_BYCOMMAND | MF_UNCHECKED );
		pMenu->CheckMenuItem( ID_TEXTURES_TEXTUREWINDOWSCALE_200, MF_BYCOMMAND | MF_UNCHECKED );
		pMenu->CheckMenuItem( id, MF_BYCOMMAND | MF_CHECKED );
	}

	g_PrefsDlg.SavePrefs();
	//FIXME: idMaterial
	//Texture_ResetPosition();
	Sys_UpdateWindows( W_TEXTURE );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnTexturesTexturewindowscale10()
{
	g_PrefsDlg.m_nTextureScale = 10;
	CheckTextureScale( ID_TEXTURES_TEXTUREWINDOWSCALE_10 );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnTexturesTexturewindowscale100()
{
	g_PrefsDlg.m_nTextureScale = 100;
	CheckTextureScale( ID_TEXTURES_TEXTUREWINDOWSCALE_100 );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnTexturesTexturewindowscale200()
{
	g_PrefsDlg.m_nTextureScale = 200;
	CheckTextureScale( ID_TEXTURES_TEXTUREWINDOWSCALE_200 );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnTexturesTexturewindowscale25()
{
	g_PrefsDlg.m_nTextureScale = 25;
	CheckTextureScale( ID_TEXTURES_TEXTUREWINDOWSCALE_25 );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnTexturesTexturewindowscale50()
{
	g_PrefsDlg.m_nTextureScale = 50;
	CheckTextureScale( ID_TEXTURES_TEXTUREWINDOWSCALE_50 );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnTexturesFlush()
{
	//FIXME: idMaterial
	//Texture_Flush();
	Sys_UpdateWindows( W_ALL );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnCurveOverlayClear()
{
	Patch_ClearOverlays();
	Sys_UpdateWindows( W_ALL );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnCurveOverlaySet()
{
	Patch_SetOverlays();
	Sys_UpdateWindows( W_ALL );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnCurveThicken()
{
	Undo_Start( "curve thicken" );
	Undo_AddBrushList( &selected_brushes );

	CDialogThick	dlg;
	if( dlg.DoModal() == IDOK )
	{
		Patch_Thicken( dlg.m_nAmount, ( dlg.m_bSeams != FALSE ) );
		Sys_UpdateWindows( W_ALL );
	}

	Undo_EndBrushList( &selected_brushes );
	Undo_End();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnCurveCyclecap()
{
	Patch_NaturalizeSelected( true, true );
	Sys_UpdateWindows( W_ALL );
}

void CMainFrame::OnCurveCyclecapAlt()
{
	Patch_NaturalizeSelected( true, true, true );
	Sys_UpdateWindows( W_ALL );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnCurveMatrixTranspose()
{
	Patch_Transpose();
	Sys_UpdateWindows( W_ALL );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnTexturesReloadshaders()
{
	CWaitCursor wait;
	declManager->Reload( false );
	Sys_UpdateWindows( W_ALL );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::SetEntityCheck()
{
	CMenu*	pMenu = GetMenu();
	if( pMenu )
	{
		pMenu->CheckMenuItem( ID_VIEW_ENTITIESAS_WIREFRAME, MF_BYCOMMAND | ( g_PrefsDlg.m_nEntityShowState == ENTITY_WIRE ) ? MF_CHECKED : MF_UNCHECKED );
		pMenu->CheckMenuItem( ID_VIEW_ENTITIESAS_SKINNED, MF_BYCOMMAND | ( g_PrefsDlg.m_nEntityShowState == ENTITY_SKINNED ) ? MF_CHECKED : MF_UNCHECKED );
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnShowEntities()
{
	HandlePopup( this, IDR_POPUP_ENTITY );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnViewEntitiesasSkinned()
{
	g_PrefsDlg.m_nEntityShowState = ENTITY_SKINNED;
	SetEntityCheck();
	g_PrefsDlg.SavePrefs();
	Sys_UpdateWindows( W_ALL );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnViewEntitiesasWireframe()
{
	g_PrefsDlg.m_nEntityShowState = ENTITY_WIRE;
	SetEntityCheck();
	g_PrefsDlg.SavePrefs();
	Sys_UpdateWindows( W_ALL );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnViewShowhint()
{
	if( ( g_qeglobals.d_savedinfo.exclude ^= EXCLUDE_HINT ) & EXCLUDE_HINT )
	{
		CheckMenuItem( ::GetMenu( GetSafeHwnd() ), ID_VIEW_SHOWHINT, MF_BYCOMMAND | MF_UNCHECKED );
	}
	else
	{
		CheckMenuItem( ::GetMenu( GetSafeHwnd() ), ID_VIEW_SHOWHINT, MF_BYCOMMAND | MF_CHECKED );
	}

	Sys_UpdateWindows( W_XY | W_CAMERA );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnTexturesShowall()
{
	Texture_ShowAll();
}

void CMainFrame::OnTexturesHideall()
{
	Texture_HideAll();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnPatchInspector()
{
	DoPatchInspector();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnViewOpengllighting()
{
	g_PrefsDlg.m_bGLLighting ^= 1;
	g_PrefsDlg.SavePrefs();
	CheckMenuItem
	(
		::GetMenu( GetSafeHwnd() ),
		ID_VIEW_OPENGLLIGHTING,
		MF_BYCOMMAND | ( g_PrefsDlg.m_bGLLighting ) ? MF_CHECKED : MF_UNCHECKED
	);
	Sys_UpdateWindows( W_XY | W_CAMERA );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnSelectAll()
{
	Select_AllOfType();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnViewShowcaulk()
{
	if( ( g_qeglobals.d_savedinfo.exclude ^= EXCLUDE_CAULK ) & EXCLUDE_CAULK )
	{
		CheckMenuItem( ::GetMenu( GetSafeHwnd() ), ID_VIEW_SHOWCAULK, MF_BYCOMMAND | MF_UNCHECKED );
	}
	else
	{
		CheckMenuItem( ::GetMenu( GetSafeHwnd() ), ID_VIEW_SHOWCAULK, MF_BYCOMMAND | MF_CHECKED );
	}

	Sys_UpdateWindows( W_XY | W_CAMERA );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnCurveFreeze()
{
	Patch_Freeze();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnCurveUnFreeze()
{
	Patch_UnFreeze( false );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnCurveUnFreezeAll()
{
	Patch_UnFreeze( true );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnSelectReselect()
{
	Select_Reselect();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnViewShowangles()
{
	if( ( g_qeglobals.d_savedinfo.exclude ^= EXCLUDE_ANGLES ) & EXCLUDE_ANGLES )
	{
		CheckMenuItem( ::GetMenu( GetSafeHwnd() ), ID_VIEW_SHOWANGLES, MF_BYCOMMAND | MF_UNCHECKED );
	}
	else
	{
		CheckMenuItem( ::GetMenu( GetSafeHwnd() ), ID_VIEW_SHOWANGLES, MF_BYCOMMAND | MF_CHECKED );
	}

	Sys_UpdateWindows( W_XY | W_CAMERA );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnEditSaveprefab()
{
	CFileDialog dlgFile
	(
		FALSE,
		"pfb",
		NULL,
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		"Prefab files (*.pfb)|*.pfb||",
		this
	);
	char		CurPath[1024];
	::GetCurrentDirectory( 1024, CurPath );

	dlgFile.m_ofn.lpstrInitialDir = CurPath;
	if( dlgFile.DoModal() == IDOK )
	{
		Map_SaveSelected( dlgFile.GetPathName().GetBuffer( 0 ) );
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnEditLoadprefab()
{
	CFileDialog dlgFile
	(
		TRUE,
		"pfb",
		NULL,
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		"Prefab files (*.pfb)|*.pfb||",
		this
	);
	char		CurPath[1024];
	::GetCurrentDirectory( 1024, CurPath );
	dlgFile.m_ofn.lpstrInitialDir = CurPath;
	if( dlgFile.DoModal() == IDOK )
	{
		Map_ImportFile( dlgFile.GetPathName().GetBuffer( 0 ) );
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnCurveMoreendcapsbevelsSquarebevel()
{
	Undo_Start( "square bevel" );
	Undo_AddBrushList( &selected_brushes );

	Patch_BrushToMesh( false, true, false, true );
	Sys_UpdateWindows( W_ALL );

	Undo_EndBrushList( &selected_brushes );
	Undo_End();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnCurveMoreendcapsbevelsSquareendcap()
{
	Undo_Start( "square endcap" );
	Undo_AddBrushList( &selected_brushes );

	Patch_BrushToMesh( false, false, true, true );
	Sys_UpdateWindows( W_ALL );

	Undo_EndBrushList( &selected_brushes );
	Undo_End();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnBrushPrimitivesSphere()
{
	Undo_Start( "make sphere" );
	Undo_AddBrushList( &selected_brushes );

	DoSides( false, true );

	Undo_EndBrushList( &selected_brushes );
	Undo_End();
}

extern bool g_bCrossHairs;

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnViewCrosshair()
{
	g_bCrossHairs ^= 1;
	Sys_UpdateWindows( W_XY );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnViewHideshowHideselected()
{
	Select_Hide();
	Select_Deselect();
}

void CMainFrame::OnViewHideshowHideNotselected()
{
	Select_Hide( true );
	Select_Deselect();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnViewHideshowShowhidden()
{
	Select_ShowAllHidden();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnTexturesShadersShow()
{
	//
	// g_PrefsDlg.m_bShowShaders ^= 1; CheckMenuItem (
	// ::GetMenu(GetSafeHwnd()), ID_TEXTURES_SHADERS_SHOW, MF_BYCOMMAND |
	// ((g_PrefsDlg.m_bShowShaders) ? MF_CHECKED : MF_UNCHECKED ));
	// Sys_UpdateWindows(W_TEXTURE);
	//
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnTexturesFlushUnused()
{
	//FIXME: idMaterial
	//Texture_FlushUnused();
	Sys_UpdateWindows( W_TEXTURE );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnSelectionInvert()
{
	Select_Invert();
	Sys_UpdateWindows( W_XY | W_Z | W_CAMERA );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnProjectedLight()
{
	LightEditorInit( NULL );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnShowLighttextures()
{
	g_bShowLightTextures ^= 1;
	SetToolbarButtonCheckState( m_wndToolBar, ID_SHOW_LIGHTTEXTURES, ( g_bShowLightTextures ) ? TRUE : FALSE );
	Sys_UpdateWindows( W_ALL );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnShowLightvolumes()
{
	g_bShowLightVolumes ^= 1;
	SetToolbarButtonCheckState( m_wndToolBar, ID_SHOW_LIGHTVOLUMES, ( g_bShowLightVolumes ) ? TRUE : FALSE );
	Sys_UpdateWindows( W_ALL );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnActivate( UINT nState, CWnd* pWndOther, BOOL bMinimized )
{
	CFrameWndEx::OnActivate( nState, pWndOther, bMinimized );
	if( nState != WA_INACTIVE )
	{
		common->ActivateTool( true );
		if( ::IsWindowVisible( win32.hWnd ) )
		{
			::ShowWindow( win32.hWnd, SW_HIDE );
		}

		// start playing the editor sound world
		soundSystem->SetPlayingSoundWorld( g_qeglobals.sw );
	}
	else
	{
		// DG: if the Radiant loses focus, tell the engine about it
		//     so the game window can get mouse focus (if it's running)
		common->ActivateTool( false );
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnSplinesMode()
{
	g_qeglobals.d_select_mode = sel_addpoint;
	g_splineList->clear();
	g_splineList->startEdit( true );
	showCameraInspector();
	Sys_UpdateWindows( W_ALL );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnSplinesLoad()
{
	g_splineList->load( "maps/test.camera" );
	g_splineList->buildCamera();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnSplinesSave()
{
	g_splineList->save( "maps/test.camera" );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnSplinesEdit()
{
	showCameraInspector();
	Sys_UpdateWindows( W_ALL );
}

extern void testCamSpeed();

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnSplineTest()
{
	long	start = GetTickCount();
	g_splineList->startCamera( start );

	float	cycle = g_splineList->getTotalTime();
	long	msecs = cycle * 1000;
	long	current = start;
	idVec3	lookat( 0, 0, 0 );
	idVec3	dir;

	while( current < start + msecs )
	{
		float	fov;
		g_splineList->getCameraInfo( current, g_pParentWnd->GetCamera()->Camera().origin, dir, &fov );
		g_pParentWnd->GetCamera()->Camera().angles[1] = atan2( dir[1], dir[0] ) * 180 / 3.14159;
		g_pParentWnd->GetCamera()->Camera().angles[0] = asin( dir[2] ) * 180 / 3.14159;
		g_pParentWnd->UpdateWindows( W_XY | W_CAMERA );
		current = GetTickCount();
	}

	g_splineList->setRunning( false );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnSplinesTargetPoints()
{
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnSplinesCameraPoints()
{
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnPopupNewcameraInterpolated()
{
	g_qeglobals.d_select_mode = sel_addpoint;
	g_qeglobals.selectObject = g_splineList->startNewCamera( idCameraPosition::INTERPOLATED );
	OnSplinesEdit();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnPopupNewcameraSpline()
{
	g_qeglobals.d_select_mode = sel_addpoint;
	g_qeglobals.selectObject = g_splineList->startNewCamera( idCameraPosition::SPLINE );
	OnSplinesEdit();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnPopupNewcameraFixed()
{
	g_qeglobals.d_select_mode = sel_addpoint;
	g_qeglobals.selectObject = g_splineList->startNewCamera( idCameraPosition::FIXED );
	OnSplinesEdit();
}

extern void Patch_AdjustSubdivisions( float hadj, float vadj );

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnCurveIncreaseVert()
{
	Patch_AdjustSubdivisions( 0.0f, -0.5f );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnCurveDecreaseVert()
{
	Patch_AdjustSubdivisions( 0.0f, 0.5f );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnCurveIncreaseHorz()
{
	Patch_AdjustSubdivisions( -0.5f, 0.0f );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnCurveDecreaseHorz()
{
	Patch_AdjustSubdivisions( 0.5f, 0.0f );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMainFrame::OnSelectionMoveonly()
{
	g_moveOnly ^= 1;
	SetToolbarButtonCheckState( m_wndToolBar, ID_SELECTION_MOVEONLY, ( g_moveOnly ) ? TRUE : FALSE );
}

void CMainFrame::OnSelectBrushlight()
{
	// TODO: Add your command handler code here

}

void CMainFrame::OnSelectionCombine()
{
	if( g_qeglobals.d_select_count < 2 )
	{
		MessageBox( "Must have at least two things selected.", "Can't Combine", MB_OK | MB_ICONINFORMATION );
		return;
	}

	idEditorEntity* e1 = g_qeglobals.d_select_order[0]->owner;

	if( e1 == world_entity )
	{
		MessageBox( "First selection must not be world.", "Can't Combine", MB_OK | MB_ICONINFORMATION );
		return;
	}

	idStr str;
	idMat3 mat;
	idVec3 v;
	if( e1->eclass->nShowFlags & ECLASS_LIGHT )
	{
		// copy the lights origin and rotation matrix to
		// light_origin and light_rotation
		e1->trackLightOrigin = true;
		e1->brushes.onext->trackLightOrigin = true;
		if( e1->GetVectorForKey( "origin", v ) )
		{
			e1->SetKeyVec3( "light_origin", v );
			e1->lightOrigin = v;
		}
		if( !e1->GetMatrixForKey( "rotation", mat ) )
		{
			mat.Identity();
		}
		sprintf( str, "%g %g %g %g %g %g %g %g %g", mat[0][0], mat[0][1], mat[0][2], mat[1][0], mat[1][1], mat[1][2], mat[2][0], mat[2][1], mat[2][2] );
		e1->SetKeyValue( "light_rotation", str, false );
		e1->lightRotation = mat;
	}

	bool setModel = true;
	for( idEditorBrush* b = selected_brushes.next; b != &selected_brushes; b = b->next )
	{
		if( b->owner != e1 )
		{
			if( e1->eclass->nShowFlags & ECLASS_LIGHT )
			{
				if( b->owner->GetVectorForKey( "origin", v ) )
				{
					e1->origin = b->owner->origin;
					e1->SetKeyVec3( "origin", b->owner->origin );
				}
				if( b->owner->GetMatrixForKey( "rotation", mat ) )
				{
					e1->rotation = b->owner->rotation;
					mat = b->owner->rotation;
					sprintf( str, "%g %g %g %g %g %g %g %g %g", mat[0][0], mat[0][1], mat[0][2], mat[1][0], mat[1][1], mat[1][2], mat[2][0], mat[2][1], mat[2][2] );
					e1->SetKeyValue( "rotation", str, false );
				}
				if( b->modelHandle )
				{
					e1->SetKeyValue( "model", b->owner->ValueForKey( "model" ) );
					setModel = false;
				}
				else
				{
					b->entityModel = true;
				}
			}
			Entity_UnlinkBrush( b );
			Entity_LinkBrush( e1, b );
		}
	}

	if( setModel )
	{
		e1->SetKeyValue( "model", e1->ValueForKey( "name" ) );
	}

	Select_Deselect();
	Select_Brush( g_qeglobals.d_select_order[0] );
	Sys_UpdateWindows( W_XY | W_CAMERA );
}

extern void Patch_Weld( patchMesh_t* p, patchMesh_t* p2 );
void CMainFrame::OnPatchCombine()
{
	patchMesh_t* p, *p2;
	p = p2 = NULL;
	for( idEditorBrush* b = selected_brushes.next; b != &selected_brushes; b = b->next )
	{
		if( b->pPatch )
		{
			if( p == NULL )
			{
				p = b->pPatch;
			}
			else if( p2 == NULL )
			{
				p2 = b->pPatch;
				Patch_Weld( p, p2 );
				return;
			}
		}
	}
}

void CMainFrame::OnCompileMap()
{
	char	sys[2048];
	char	name[2048];
	char*	in;

	if( g_PrefsDlg.m_bSnapShots && stricmp( currentmap, "unnamed.map" ) != 0 )
	{
		Map_Snapshot();
	}

	// bring the console window forward for feedback
	// sikk - Added so BSP doesn't hide the console
	if( !g_Inspectors->IsWindowVisible() || g_Inspectors->prevMode != W_CONSOLE )
	{
		g_Inspectors->SetMode( W_CONSOLE );
	}

	// decide if we are doing a .map or a .reg
	strcpy( name, currentmap );
	if( region_active )
	{
		Map_SaveFile( name, false );
		StripExtension( name );
		strcat( name, ".reg" );
	}

	if( !Map_SaveFile( name, region_active ) )
	{
		return;
	}

	// name should be a full pathname, but we only
	// want to pass the maps/ part to dmap
	in = strstr( name, "maps/" );
	if( !in )
	{
		in = strstr( name, "maps\\" );
	}

	if( !in )
	{
		in = name;
	}

	idStr::snPrintf( sys, sizeof( sys ), "dmap %s", in );

	cmdSystem->BufferCommandText( CMD_EXEC_NOW, "disconnect\n" );

	// issue the bsp command
	Dmap_f( idCmdArgs( sys, false ) );

	SetTimer( QE_TIMER1, g_PrefsDlg.m_nAutoSave * 60 * 1000, NULL );
}

void CMainFrame::OnShowDoom()
{
	int show = ::IsWindowVisible( win32.hWnd ) ? SW_HIDE : SW_NORMAL;
	if( show == SW_NORMAL )
	{
		g_Inspectors->SetMode( W_TEXTURE );
	}
	::ShowWindow( win32.hWnd, show );

	cmdSystem->BufferCommandText( CMD_EXEC_NOW, "disconnect" );
	cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "devmap %s", fileSystem->OSPathToRelativePath( currentmap ) + strlen( "maps\\" ) ) );
}

void CMainFrame::OnViewRendermode()
{
	m_pCamWnd->ToggleRenderMode();
	CheckMenuItem( ::GetMenu( GetSafeHwnd() ), ID_VIEW_RENDERMODE, MF_BYCOMMAND | ( m_pCamWnd->GetRenderMode() ) ? MF_CHECKED : MF_UNCHECKED );
	Sys_UpdateWindows( W_ALL );
}

void CMainFrame::OnViewRebuildrenderdata()
{
	m_pCamWnd->BuildRendererState();
	if( !m_pCamWnd->GetRenderMode() )
	{
		OnViewRendermode();
	}
	Sys_UpdateWindows( W_ALL );
}

void CMainFrame::OnViewRealtimerebuild()
{
	m_pCamWnd->ToggleRebuildMode();
	CheckMenuItem( ::GetMenu( GetSafeHwnd() ), ID_VIEW_REALTIMEREBUILD, MF_BYCOMMAND | ( m_pCamWnd->GetRebuildMode() ) ? MF_CHECKED : MF_UNCHECKED );
	Sys_UpdateWindows( W_ALL );
}

void CMainFrame::OnViewRenderentityoutlines()
{
	m_pCamWnd->ToggleEntityMode();
	CheckMenuItem( ::GetMenu( GetSafeHwnd() ), ID_VIEW_RENDERENTITYOUTLINES, MF_BYCOMMAND | ( m_pCamWnd->GetEntityMode() ) ? MF_CHECKED : MF_UNCHECKED );
	Sys_UpdateWindows( W_ALL );
}

void CMainFrame::OnViewMaterialanimation()
{
	m_pCamWnd->ToggleAnimationMode();
	CheckMenuItem( ::GetMenu( GetSafeHwnd() ), ID_VIEW_MATERIALANIMATION, MF_BYCOMMAND | ( m_pCamWnd->GetAnimationMode() ) ? MF_CHECKED : MF_UNCHECKED );
	Sys_UpdateWindows( W_ALL );
}

extern void Face_SetAxialScale_BrushPrimit( face_t* face, bool y );
void CMainFrame::OnAxialTextureByWidth()
{
	// temp test code
	int faceCount = g_ptrSelectedFaces.GetSize();

	if( faceCount > 0 )
	{
		for( int i = 0; i < faceCount; i++ )
		{
			face_t*	selFace = reinterpret_cast < face_t* >( g_ptrSelectedFaces.GetAt( i ) );
			Face_SetAxialScale_BrushPrimit( selFace, false );
		}
		Sys_UpdateWindows( W_CAMERA );
	}

}

void CMainFrame::OnAxialTextureByHeight()
{
	// temp test code
	int faceCount = g_ptrSelectedFaces.GetSize();

	if( faceCount > 0 )
	{
		for( int i = 0; i < faceCount; i++ )
		{
			face_t*	selFace = reinterpret_cast < face_t* >( g_ptrSelectedFaces.GetAt( i ) );
			Face_SetAxialScale_BrushPrimit( selFace, true );
		}
		Sys_UpdateWindows( W_CAMERA );
	}
}

void CMainFrame::OnAxialTextureArbitrary()
{
	if( g_bAxialMode )
	{
		g_bAxialMode = false;
	}
	int faceCount = g_ptrSelectedFaces.GetSize();
	if( faceCount > 0 )
	{
		g_axialAnchor = 0;
		g_axialDest = 1;
		g_bAxialMode = true;
	}
	Sys_UpdateWindows( W_CAMERA );
}

extern void Select_ToOBJ();
void CMainFrame::OnSelectionExportToobj()
{
	Select_ToOBJ();
}

extern void Select_ToCM();
void CMainFrame::OnSelectionExportToCM()
{
	Select_ToCM();
}

void CMainFrame::OnSelectionWireFrameOff()
{
	Select_WireFrame( false );
}

void CMainFrame::OnSelectionWireFrameOn()
{
	Select_WireFrame( true );
}

void CMainFrame::OnSelectionVisibleOn()
{
	Select_ForceVisible( true );
}

void CMainFrame::OnSelectionVisibleOff()
{
	Select_ForceVisible( false );
}


void CMainFrame::OnViewRenderselection()
{
	m_pCamWnd->ToggleSelectMode();
	CheckMenuItem( ::GetMenu( GetSafeHwnd() ), ID_VIEW_RENDERSELECTION, MF_BYCOMMAND | ( m_pCamWnd->GetSelectMode() ) ? MF_CHECKED : MF_UNCHECKED );
	Sys_UpdateWindows( W_CAMERA );
}

void CMainFrame::OnSelectNomodels()
{
	g_PrefsDlg.m_selectNoModels ^= 1;
	SetToolbarButtonCheckState( m_wndToolBar, ID_SELECT_NOMODELS, ( g_PrefsDlg.m_selectNoModels ) ? TRUE : FALSE );
}

void CMainFrame::OnViewShowShowvisportals()
{
	if( ( g_qeglobals.d_savedinfo.exclude ^= EXCLUDE_VISPORTALS ) & EXCLUDE_VISPORTALS )
	{
		CheckMenuItem( ::GetMenu( GetSafeHwnd() ), ID_VIEW_SHOW_SHOWVISPORTALS, MF_BYCOMMAND | MF_UNCHECKED );
	}
	else
	{
		CheckMenuItem( ::GetMenu( GetSafeHwnd() ), ID_VIEW_SHOW_SHOWVISPORTALS, MF_BYCOMMAND | MF_CHECKED );
	}

	Sys_UpdateWindows( W_XY | W_CAMERA );
}

void CMainFrame::OnViewShowNoDraw()
{
	if( ( g_qeglobals.d_savedinfo.exclude ^= EXCLUDE_NODRAW ) & EXCLUDE_NODRAW )
	{
		CheckMenuItem( ::GetMenu( GetSafeHwnd() ), ID_VIEW_SHOW_NODRAW, MF_BYCOMMAND | MF_UNCHECKED );
	}
	else
	{
		CheckMenuItem( ::GetMenu( GetSafeHwnd() ), ID_VIEW_SHOW_NODRAW, MF_BYCOMMAND | MF_CHECKED );
	}

	Sys_UpdateWindows( W_XY | W_CAMERA );
}



void CMainFrame::OnViewRendersound()
{
	m_pCamWnd->ToggleSoundMode();
	CheckMenuItem( ::GetMenu( GetSafeHwnd() ), ID_VIEW_RENDERSOUND, MF_BYCOMMAND | ( m_pCamWnd->GetSoundMode() ) ? MF_CHECKED : MF_UNCHECKED );
	Sys_UpdateWindows( W_CAMERA );
}


void CMainFrame::OnSoundShowsoundvolumes()
{
	g_qeglobals.d_savedinfo.showSoundAlways ^= 1;
	if( g_qeglobals.d_savedinfo.showSoundAlways )
	{
		g_qeglobals.d_savedinfo.showSoundWhenSelected = false;
	}
	SetToolbarButtonCheckState( m_wndToolBar, ID_SOUND_SHOWSOUNDVOLUMES, g_qeglobals.d_savedinfo.showSoundAlways );
	SetToolbarButtonCheckState( m_wndToolBar, ID_SOUND_SHOWSELECTEDSOUNDVOLUMES, g_qeglobals.d_savedinfo.showSoundWhenSelected );
	Sys_UpdateWindows( W_XY | W_CAMERA );
}

void CMainFrame::OnNurbEditor()
{
	nurbMode ^= 1;
	if( nurbMode )
	{
		int num = nurb.GetNumValues();
		idStr temp = va( "%i 3 ", num );
		for( int i = 0; i < num; i++ )
		{
			temp += va( "(%i %i) ", ( int )nurb.GetValue( i ).x, ( int )nurb.GetValue( i ).y );
		}
		temp += "\r\n";
		if( OpenClipboard() )
		{
			::EmptyClipboard();
			HGLOBAL clip;
			char* buff;
			clip = ::GlobalAlloc( GMEM_DDESHARE, temp.Length() + 1 );
			buff = ( char* )::GlobalLock( clip );
			strcpy( buff, temp );
			::GlobalUnlock( clip );
			::SetClipboardData( CF_TEXT, clip );
			::CloseClipboard();
		}
		nurb.Clear();
	}
}


void CMainFrame::OnSoundShowselectedsoundvolumes()
{
	g_qeglobals.d_savedinfo.showSoundWhenSelected ^= 1;
	if( g_qeglobals.d_savedinfo.showSoundWhenSelected )
	{
		g_qeglobals.d_savedinfo.showSoundAlways = false;
	}
	SetToolbarButtonCheckState( m_wndToolBar, ID_SOUND_SHOWSOUNDVOLUMES, g_qeglobals.d_savedinfo.showSoundAlways );
	SetToolbarButtonCheckState( m_wndToolBar, ID_SOUND_SHOWSELECTEDSOUNDVOLUMES, g_qeglobals.d_savedinfo.showSoundWhenSelected );
	Sys_UpdateWindows( W_XY | W_CAMERA );
}

void CMainFrame::OnSelectAlltargets()
{
	Select_AllTargets();
}


void CMainFrame::OnSelectCompleteEntity()
{
	idEditorBrush* b = NULL;
	idEditorEntity* e = NULL;

	b = selected_brushes.next;
	if( b == &selected_brushes )
	{
		return; //no brushes selected
	}

	e = b->owner;
	if( b->owner == world_entity )
	{
		return; //don't select the world entity
	}

	for( b = e->brushes.onext; b != &e->brushes; b = b->onext )
	{
		Select_Brush( b , false );
	}
	Sys_UpdateWindows( W_ALL );
}




//---------------------------------------------------------------------------
// OnPrecisionCursorCycle
//
// Called when the user presses the "cycle precision cursor mode" key.
// Cycles the precision cursor among the following three modes:
//		PRECISION_CURSOR_NONE
//		PRECISION_CURSOR_SNAP
//		PRECISION_CURSOR_FREE
//---------------------------------------------------------------------------
void CMainFrame::OnPrecisionCursorCycle()
{
	m_pActiveXY->CyclePrecisionCrosshairMode();
}

void CMainFrame::OnGenerateMaterialsList()
{
	idStrList mtrList;
	idStr     mtrName, mtrFileName;


	g_Inspectors->consoleWnd.ExecuteCommand( "clear" );
	Sys_BeginWait();
	common->Printf( "Generating list of active materials...\n" );

	for( idEditorBrush* b = active_brushes.next ; b != &active_brushes ; b = b->next )
	{
		if( b->pPatch )
		{
			mtrName = b->pPatch->d_texture->GetName();
			if( !mtrList.Find( mtrName ) )
			{
				mtrList.Insert( mtrName );
			}

		}
		else
		{
			for( face_t* f = b->brush_faces ; f != NULL ; f = f->next )
			{
				mtrName = f->d_texture->GetName();
				if( !mtrList.Find( mtrName ) )
				{
					mtrList.Insert( mtrName );
				}

			}
		}
	}

	mtrList.Sort();
	for( int i = 0 ; i < mtrList.Num() ; i++ )
	{
		common->Printf( "%s\n" , mtrList[i].c_str() );
	}

	mtrFileName = currentmap;
//	mtrFileName.ExtractFileName( mtrFileName );
	mtrFileName = mtrFileName.StripPath();

	common->Printf( "Done...found %i unique materials\n" , mtrList.Num() );
	mtrFileName = mtrFileName + idStr( "_materials.txt" );

	// bring the console window forward for feedback
	// sikk - Added so BSP doesn't hide the console
	if( !g_Inspectors->IsWindowVisible() || g_Inspectors->prevMode != W_CONSOLE )
	{
		g_Inspectors->SetMode( W_CONSOLE );
	}

	cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "condump %s\n" , mtrFileName.c_str() ) );

	Sys_EndWait();
}

/*
=======================================================================================================================
=======================================================================================================================
*/


void CMainFrame::OnSplinesAddPoints()
{
	g_Inspectors->entityDlg.AddCurvePoints();
}

void CMainFrame::OnSplinesEditPoints()
{
	g_Inspectors->entityDlg.EditCurvePoints();
}

void CMainFrame::OnSplinesDeletePoint()
{
	g_Inspectors->entityDlg.DeleteCurvePoint();
}

void CMainFrame::OnSplinesInsertPoint()
{
	g_Inspectors->entityDlg.InsertCurvePoint();
}
