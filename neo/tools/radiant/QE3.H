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

#ifndef __QE3_H__
#define __QE3_H__

#ifdef ID_DEBUG_MEMORY
	#undef new
	#undef DEBUG_NEW
	#define DEBUG_NEW new
#endif

#include "QerTypes.h"
#include "../common/CMDLib.h"
#include "../common/Parse.h"

#include <commctrl.h>
#include "afxres.h"
#include "../../sys/win32/rc/resource.h"       // main symbols
#include "../../sys/win32/win_local.h"

#include "QeDefs.h"

// stuff from old qfiles.h
const int MAX_MAP_ENTITIES = 2048;
const int MAX_MOVE_POINTS = 4096;
const int MAX_MOVE_PLANES = 2048;

// assume that all maps fit within this
const float	HUGE_DISTANCE = 100000;

#include "textures.h"
#include "EditorBrush.h"
#include "EditorEntity.h"
#include "EditorMap.h"
#include "select.h"
#include "splines.h"

#include "z.h"
#include "mru.h"
#include "waitdlg.h"
#include "MainFrm.h"
#include "PrefsDlg.h"
#include "FindTextureDlg.h"
#include "dialogtextures.h"
#include "InspectorDialog.h"
#include "undo.h"
#include "PMESH.H"
#include "SplashScreen.h"
#include "../common/AboutBoxDlg.h"

typedef struct
{
	int		p1, p2;
	face_t*	f1, *f2;
} pedge_t;

typedef struct
{
	int		iSize;
	int		iTexMenu;		// nearest, linear, etc
	idVec3	colors[COLOR_LAST];
	bool	show_names;
	bool	show_coordinates;
	int		exclude;
	float	m_nTextureTweak;
	bool	editorExpanded;
	RECT	oldEditRect;
	bool	showSoundAlways;
	bool	showSoundWhenSelected;
} SavedInfo_t;

//
// system functions
//
// TTimo NOTE: WINAPI funcs can be accessed by plugins
void    Sys_UpdateStatusBar();
void    Sys_UpdateWindows( int bits );
double	Sys_DoubleTime();
void    Sys_GetCursorPos( int* x, int* y );
void    Sys_SetCursorPos( int x, int y );
void    Sys_SetTitle( const char* text );
void    Sys_BeginWait();
bool	Sys_Waiting();
void    Sys_EndWait();
void    Sys_Status( const char* psz, int part = -1 );

/*
** most of the QE globals are stored in this structure
*/
typedef struct
{
	bool d_showgrid;
	float d_gridsize;
	float d_selectedgridsize;

	int	rotateAxis;			// 0, 1 or 2
	int flatRotation;		// 0, 1 or 2, 0 == off, 1 == rotate about the rotation origin, 1 == rotate about the selection mid point

	int      d_num_entities;

	idVec3	d_new_brush_bottom, d_new_brush_top;

	HINSTANCE d_hInstance;

	idVec3    d_points[MAX_POINTS];
	int       d_numpoints;
	pedge_t   d_edges[MAX_EDGES];
	int       d_numedges;

	int       d_num_move_points;
	idVec3*    d_move_points[MAX_MOVE_POINTS];

	int			d_num_move_planes;
	idPlane*		d_move_planes[MAX_MOVE_PLANES];

	texturewin_t d_texturewin;

	LPMRUMENU    d_lpMruMenu;

	SavedInfo_t  d_savedinfo;

	// connect entities uses the last two brushes selected
	int			 d_select_count;
	idEditorBrush*		d_select_order[MAX_MAP_ENTITIES];
	idVec3       d_select_translate;    // for dragging w/o making new display lists
	select_t     d_select_mode;
	idPointListInterface* selectObject;	//

	int		     d_font_list;

	int          d_parsed_brushes;

	bool	show_blocks;

	// used while importing brush data from file or memory buffer
	// tells if conversion between map format and internal preferences ( m_bBrushPrimitMode ) is needed
	bool	bNeedConvert;
	bool	bOldBrushes;
	bool	bPrimitBrushes;
	float	mapVersion;

	idVec3  d_vAreaTL;
	idVec3  d_vAreaBR;

	// the editor has its own soundWorld and renderWorld, completely distinct from the game
	idRenderWorld*	rw;
	idSoundWorld*	sw;
} QEGlobals_t;

//=========================
// pointfile.cpp
//========================
void Pointfile_Delete();
void Pointfile_Check();
void Pointfile_Next();
void Pointfile_Prev();
void Pointfile_Clear();
void Pointfile_Draw();

//========================
// drag.c
//========================
void Drag_Begin( int x, int y, int buttons,
				 const idVec3& xaxis, const idVec3& yaxis,
				 const idVec3& origin, const idVec3& dir );
void Drag_MouseMoved( int x, int y, int buttons );
void Drag_MouseUp( int nButtons = 0 );
extern bool g_moveOnly;

//========================
// csg.c
//========================
void CSG_MakeHollow();
void CSG_Subtract();
void CSG_Merge();

//========================
// vertsel.c
//========================
void SetupVertexSelection();
void SelectEdgeByRay( idVec3 org, idVec3 dir );
void SelectVertexByRay( idVec3 org, idVec3 dir );

void ConnectEntities();

//========================
// win_main.c
//========================
extern bool SaveWindowState( HWND hWnd, const char* pszName );
extern bool LoadWindowState( HWND hWnd, const char* pszName );

extern bool SaveRegistryInfo( const char* pszName, void* pvBuf, long lSize );
extern bool LoadRegistryInfo( const char* pszName, void* pvBuf, long* plSize );

//========================
// win_dlg.c
//========================
void DoSurface();

//========================
//	win_snap.cpp
//
//	*ripped from QE5
//========================
bool FindClosestBottom( HWND h, LPRECT rect, LPRECT parentrect );
bool FindClosestTop( HWND h, LPRECT rect, LPRECT parentrect );
bool FindClosestLeft( HWND h, LPRECT rect, LPRECT parentrect, int widthlimit );
bool FindClosestRight( HWND h, LPRECT rect, LPRECT parentrect, int widthlimit );
bool FindClosestHorizontal( HWND h, LPRECT rect, LPRECT parentrect );
bool FindClosestVertical( HWND h, LPRECT rect, LPRECT parentrect );
bool TryDocking( HWND h, long side, LPRECT rect, int widthlimit );

//========================
// QE function declarations
//========================
void	QE_CheckAutoSave();
void	QE_CountBrushesAndUpdateStatusBar();
void	WINAPI QE_CheckOpenGLForErrors();
void	QE_Init();
bool	QE_SingleBrush( bool bQuiet = false, bool entityOK = false );


//========================
// sys stuff
//========================
void Sys_MarkMapModified();

//========================
// QE Win32 function declarations
//========================
int  WINAPI QEW_SetupPixelFormat( HDC hDC, bool zbuffer );

//========================
// extern declarations
//========================
extern QEGlobals_t   g_qeglobals;
extern int mapModified;		// for quit confirmation (0 = clean, 1 = unsaved,

extern bool g_bAxialMode;
extern int g_axialAnchor;
extern int g_axialDest;

extern void Face_FlipTexture_BrushPrimit( face_t* face, bool y );
extern void Brush_FlipTexture_BrushPrimit( idEditorBrush* b, bool y );

//========================
// new brush primitive stuff
// Timo
//========================
//void ComputeAxisBase( idVec3 &normal,idVec3 &texS,idVec3 &texT );
void FaceToBrushPrimitFace( face_t* f );
void EmitBrushPrimitTextureCoordinates( face_t*, idWinding*, patchMesh_t* patch = NULL );
// EmitTextureCoordinates, is old code used for brush to brush primitive conversion
void EmitTextureCoordinates( idVec5& xyzst, const idMaterial* q, face_t* f, bool force = false );
void BrushPrimit_Parse( idEditorBrush*, bool newFormat, const idVec3 origin );
// compute a fake shift scale rot representation from the texture matrix
void TexMatToFakeTexCoords( float texMat[2][3], float shift[2], float* rot, float scale[2] );
void FakeTexCoordsToTexMat( float shift[2], float rot, float scale[2], float texMat[2][3] );
void ConvertTexMatWithQTexture( brushprimit_texdef_t* texMat1, const idMaterial* qtex1, brushprimit_texdef_t* texMat2, const idMaterial* qtex2, float sScale = 1.0, float tScale = 1.0 );
// texture locking
void Face_MoveTexture_BrushPrimit( face_t* f, idVec3 delta );
void Select_ShiftTexture_BrushPrimit( face_t* f, float x, float y, bool autoAdjust );
void RotateFaceTexture_BrushPrimit( face_t* f, int nAxis, float fDeg, idVec3 vOrigin );

void ApplyMatrix_BrushPrimit( face_t* f, idMat3 matrix, idVec3 origin );
// low level functions .. put in mathlib?
#define BPMatCopy(a,b) {b[0][0] = a[0][0]; b[0][1] = a[0][1]; b[0][2] = a[0][2]; b[1][0] = a[1][0]; b[1][1] = a[1][1]; b[1][2] = a[1][2];}
// apply a scale transformation to the BP matrix
#define BPMatScale(m,sS,sT) {m[0][0]*=sS; m[1][0]*=sS; m[0][1]*=sT; m[1][1]*=sT;}
// apply a translation transformation to a BP matrix
#define BPMatTranslate(m,s,t) {m[0][2] += m[0][0]*s + m[0][1]*t; m[1][2] += m[1][0]*s+m[1][1]*t;}
// 2D homogeneous matrix product C = A*B
void BPMatMul( float A[2][3], float B[2][3], float C[2][3] );
// apply a rotation (degrees)
void BPMatRotate( float A[2][3], float theta );
#ifdef _DEBUG
	void BPMatDump( float A[2][3] );
#endif

extern bool g_bShowLightVolumes;
extern bool g_bShowLightTextures;
extern const idMaterial* Texture_LoadLight( const char* name );

#define	FONT_HEIGHT	10

void UniqueTargetName( idStr& rStr );

#define MAP_VERSION 2.0

extern CMainFrame* g_pParentWnd;
extern CString g_strAppPath;
extern CPrefsDlg& g_PrefsDlg;
extern CFindTextureDlg& g_dlgFind;
extern idCVar radiant_entityMode;

// externs
extern void CleanUpEntities();
extern void QE_CountBrushesAndUpdateStatusBar();
extern void	QE_CheckAutoSave();
extern bool SaveWindowState( HWND hWnd, const char* pszName );
extern void Map_Snapshot();
extern bool g_bRotateMode;
extern bool g_bClipMode;
extern bool g_bScaleMode;
extern int g_nScaleHow;
extern bool g_bPathMode;

extern void FindReplaceTextures( const char* pFind, const char* pReplace, bool bSelected, bool bForce );
extern bool region_active;
extern void Map_ImportFile( char* filename );
extern void Map_SaveSelected( char* pFilename );
extern bool g_bSwitch;
extern idEditorBrush g_brFrontSplits;
extern idEditorBrush g_brBackSplits;
extern CClipPoint g_Clip1;
extern CClipPoint g_Clip2;
extern CClipPoint g_Clip3;
extern CClipPoint* g_pMovingClip;
extern idEditorBrush* g_pSplitList;
extern CClipPoint g_PathPoints[256];
extern void AcquirePath( int nCount, PFNPathCallback* pFunc );
extern bool g_bScreenUpdates;
extern SCommandInfo g_Commands[];
extern int g_nCommandCount;
extern SKeyInfo g_Keys[];
extern int g_nKeyCount;
extern void HandlePopup( CWnd* pWindow, unsigned int uId );
extern z_t z;
extern void Clamp( float& f, int nClamp );
extern bool WriteFileString( FILE* fp, char* string, ... );
extern void MemFile_fprintf( CMemFile* pMemFile, const char* pText, ... );
extern void SaveWindowPlacement( HWND hwnd, const char* pName );
extern bool LoadWindowPlacement( HWND hwnd, const char* pName );

void SaveDialogPlacement( CDialog* dlg, const char* pName );
extern bool ConfirmModified();
extern void DoPatchInspector();
void UpdatePatchInspector();
extern int g_nSmartX;
extern int g_nSmartY;
extern idEditorBrush* CreateEntityBrush( int x, int y, CXYWnd* pWnd );
int PointInMoveList( idVec3* pf );

extern bool ByeByeSurfaceDialog();
extern void UpdateSurfaceDialog();
extern void UpdateLightInspector();

int GetCvarInt( const char* name, const int def );
const char* GetCvarString( const char* name, const char* def );
void SetCvarInt( const char* name, const int value );
void SetCvarString( const char* name, const char* value );
void SetCvarBinary( const char* name, void* pv, int size );
bool GetCvarBinary( const char* name, void* pv, int size );

void UpdateRadiantColor( float r, float g, float b, float a );

extern std::chrono::steady_clock::time_point lastFrameTime;

#endif /* !__QE3_H__ */
