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
#include "Radiant.h"
#include "XYWnd.h"
#include "CamWnd.h"
#include "splines.h"
#include <GL/glu.h>

#include "../../renderer/tr_local.h"
#include "../../renderer/model_local.h" // for idRenderModelMD5

// TODO: DG: could merge SteelStorm2 "new 3D view navigation" improvements

#ifdef _DEBUG
	#define new DEBUG_NEW
#endif
extern void DrawPathLines();

int			g_axialAnchor = -1;
int			g_axialDest	  = -1;
bool		g_bAxialMode  = false;

void		ValidateAxialPoints()
{
	int faceCount = g_ptrSelectedFaces.GetSize();
	if( faceCount > 0 )
	{
		face_t* selFace = reinterpret_cast<face_t*>( g_ptrSelectedFaces.GetAt( 0 ) );
		if( g_axialAnchor >= selFace->face_winding->GetNumPoints() )
		{
			g_axialAnchor = 0;
		}
		if( g_axialDest >= selFace->face_winding->GetNumPoints() )
		{
			g_axialDest = 0;
		}
	}
	else
	{
		g_axialDest	  = 0;
		g_axialAnchor = 0;
	}
}

// CCamWnd
IMPLEMENT_DYNCREATE( CCamWnd, CDialogEx );

/*
 =======================================================================================================================
 =======================================================================================================================
 */
CCamWnd::CCamWnd()
{
	memset( &m_Camera, 0, sizeof( camera_t ) );
	m_pXYFriend	   = NULL;
	m_pSide_select = NULL;
	m_bClipMode	   = false;
	worldDirty	   = true;
	worldModel	   = NULL;
	renderMode	   = false;
	rebuildMode	   = false;
	entityMode	   = false;
	animationMode  = false;
	selectMode	   = false;
	soundMode	   = false;
	saveValid	   = false;
	Cam_Init();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
CCamWnd::~CCamWnd()
{
}

BEGIN_MESSAGE_MAP( CCamWnd, CDialogEx )
//{{AFX_MSG_MAP(CCamWnd)
ON_WM_KEYDOWN()
ON_WM_PAINT()
ON_WM_DESTROY()
ON_WM_CLOSE()
ON_WM_MOUSEMOVE()
ON_WM_LBUTTONDOWN()
ON_WM_LBUTTONUP()
ON_WM_MBUTTONDOWN()
ON_WM_MBUTTONUP()
ON_WM_RBUTTONDOWN()
ON_WM_RBUTTONUP()
ON_WM_CREATE()
ON_WM_SIZE()
ON_WM_KEYUP()
ON_WM_NCCALCSIZE()
ON_WM_KILLFOCUS()
ON_WM_SETFOCUS()
ON_WM_TIMER()
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

//
// =======================================================================================================================
//    CCamWnd message handlers
// =======================================================================================================================
//
BOOL CCamWnd::PreCreateWindow( CREATESTRUCT& cs )
{
	cs.dwExStyle = WS_EX_TOOLWINDOW;
	return CDialogEx::PreCreateWindow( cs );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CCamWnd::OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags )
{
	g_pParentWnd->HandleKey( nChar, nRepCnt, nFlags );
}

idEditorBrush* g_pSplitList = NULL;

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void		   CCamWnd::OnPaint()
{
	CPaintDC dc( this ); // device context for painting

	if( !qwglMakeCurrent( dc.m_hDC, win32.hGLRC ) )
	{
		common->Printf( "ERROR: qwglMakeCurrent failed..\n " );
		common->Printf( "Please restart " EDITOR_WINDOWTEXT " if the camera view is not working\n" );
	}
	else
	{
		QE_CheckOpenGLForErrors();
		g_pSplitList = NULL;
		if( g_bClipMode )
		{
			if( g_Clip1.Set() && g_Clip2.Set() )
			{
				g_pSplitList = ( ( g_pParentWnd->ActiveXY()->GetViewType() == ViewType::XZ ) ? !g_bSwitch : g_bSwitch ) ? &g_brBackSplits : &g_brFrontSplits;
			}
		}

		Cam_Draw();
		QE_CheckOpenGLForErrors();
		qwglSwapBuffers( dc.m_hDC );
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CCamWnd::SetXYFriend( CXYWnd* pWnd )
{
	m_pXYFriend = pWnd;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CCamWnd::OnDestroy()
{
	SaveDialogPlacement( this, "radiant_camerawindow" );
	CDialogEx::OnDestroy();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CCamWnd::OnClose()
{
	CDialogEx::OnClose();
}

extern void Select_RotateTexture( float amt, bool absolute );

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void		CCamWnd::OnMouseMove( UINT nFlags, CPoint point )
{
	CRect r;
	if( m_bMouseLook )
	{
		CPoint currentPos;
		GetCursorPos( &currentPos );
		float dx = ( float )( currentPos.x - m_LastMousePos.x );
		float dy = ( float )( currentPos.y - m_LastMousePos.y );
		UpdateCameraOrientation( dx, dy );
		SetCursorPos( m_LastMousePos.x, m_LastMousePos.y ); // Reset cursor position
	}
	else
	{
		GetClientRect( r );
		if( GetCapture() == this && ( GetAsyncKeyState( VK_MENU ) & 0x8000 ) && !( ( GetAsyncKeyState( VK_SHIFT ) & 0x8000 ) || ( GetAsyncKeyState( VK_CONTROL ) & 0x8000 ) ) )
		{
			if( GetAsyncKeyState( VK_CONTROL ) & 0x8000 )
			{
				Select_RotateTexture( ( float )point.y - m_ptLastCursor.y );
			}
			else if( GetAsyncKeyState( VK_SHIFT ) & 0x8000 )
			{
				Select_ScaleTexture( ( float )point.x - m_ptLastCursor.x, ( float )m_ptLastCursor.y - point.y );
			}
			else
			{
				Select_ShiftTexture( ( float )point.x - m_ptLastCursor.x, ( float )m_ptLastCursor.y - point.y );
			}
		}
		else
		{
			Cam_MouseMoved( point.x, r.bottom - 1 - point.y, nFlags );
		}

		m_ptLastCursor = point;
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CCamWnd::OnLButtonDown( UINT nFlags, CPoint point )
{
	m_ptLastCursor = point;
	OriginalMouseDown( nFlags, point );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CCamWnd::OnLButtonUp( UINT nFlags, CPoint point )
{
	OriginalMouseUp( nFlags, point );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CCamWnd::OnMButtonDown( UINT nFlags, CPoint point )
{
	OriginalMouseDown( nFlags, point );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CCamWnd::OnMButtonUp( UINT nFlags, CPoint point )
{
	OriginalMouseUp( nFlags, point );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CCamWnd::OnRButtonDown( UINT nFlags, CPoint point )
{
	EnableMouseLook( true );
	CDialogEx::OnRButtonDown( nFlags, point );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CCamWnd::OnRButtonUp( UINT nFlags, CPoint point )
{
	EnableMouseLook( false );
	CDialogEx::OnRButtonUp( nFlags, point );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
int CCamWnd::OnCreate( LPCREATESTRUCT lpCreateStruct )
{
	if( CDialogEx::OnCreate( lpCreateStruct ) == -1 )
	{
		return -1;
	}

	CDC* pDC = GetDC();
	HDC	 hDC = pDC->GetSafeHdc();

	QEW_SetupPixelFormat( hDC, true );

	HFONT hfont = CreateFont( 12, // logical height of font
		0,						  // logical average character width
		0,						  // angle of escapement
		0,						  // base-line orientation angle
		0,						  // font weight
		0,						  // italic attribute flag
		0,						  // underline attribute flag
		0,						  // strikeout attribute flag
		0,						  // character set identifier
		0,						  // output precision
		0,						  // clipping precision
		0,						  // output quality
		FIXED_PITCH | FF_MODERN,  // pitch and family
		"MS Shell Dlg"			  // pointer to typeface name string
	);

	if( !hfont )
	{
		idLib::Error( "couldn't create font" );
	}

	HFONT hOldFont = ( HFONT )SelectObject( hDC, hfont );

	// qwglMakeCurrent (hDC, win32.hGLRC);
	if( qwglMakeCurrent( hDC, win32.hGLRC ) == FALSE )
	{
		common->Warning( "qwglMakeCurrent failed: %d", ::GetLastError() );
		if( r_multiSamples.GetInteger() > 0 )
		{
			common->Warning( "\n!!! Try setting r_multiSamples 0 when using the editor !!!\n" );
		}
	}

	if( ( g_qeglobals.d_font_list = qglGenLists( 256 ) ) == 0 )
	{
		common->Warning( "couldn't create font dlists" );
	}

	// create the bitmap display lists we're making images of glyphs 0 thru 255
	if( !qwglUseFontBitmaps( hDC, 0, 255, g_qeglobals.d_font_list ) )
	{
		common->Warning( "qwglUseFontBitmaps failed (%d).  Trying again.", GetLastError() );

		// FIXME: This is really wacky, sometimes the first call fails, but calling it again makes it work
		//		This probably indicates there's something wrong somewhere else in the code, but I'm not sure what
		if( !qwglUseFontBitmaps( hDC, 0, 255, g_qeglobals.d_font_list ) )
		{
			common->Warning( "qwglUseFontBitmaps failed again (%d).  Trying outlines.", GetLastError() );

			if( !qwglUseFontOutlines( hDC, 0, 255, g_qeglobals.d_font_list, 0.0f, 0.1f, WGL_FONT_LINES, NULL ) )
			{
				common->Warning( "qwglUseFontOutlines also failed (%d), no coordinate text will be visible.", GetLastError() );
			}
		}
	}

	SelectObject( hDC, hOldFont );
	ReleaseDC( pDC );

	// indicate start of glyph display lists
	qglListBase( g_qeglobals.d_font_list );

	SetWindowTheme( GetSafeHwnd(), L"DarkMode_Explorer", NULL );

	// report OpenGL information
#ifdef _DEBUG
	common->Printf( "GL_VENDOR: %s\n", qglGetString( GL_VENDOR ) );
	common->Printf( "GL_RENDERER: %s\n", qglGetString( GL_RENDERER ) );
	common->Printf( "GL_VERSION: %s\n", qglGetString( GL_VERSION ) );
	common->Printf( "GL_EXTENSIONS: %s\n", qglGetString( GL_EXTENSIONS ) );
#endif // _DEBUG

	return 0;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CCamWnd::OriginalMouseUp( UINT nFlags, CPoint point )
{
	CRect r;
	GetClientRect( r );
	Cam_MouseUp( point.x, r.bottom - 1 - point.y, nFlags );
	if( !( nFlags & ( MK_LBUTTON | MK_RBUTTON | MK_MBUTTON ) ) )
	{
		ReleaseCapture();
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CCamWnd::OriginalMouseDown( UINT nFlags, CPoint point )
{
	// if (GetTopWindow()->GetSafeHwnd() != GetSafeHwnd()) BringWindowToTop();
	CRect r;
	GetClientRect( r );
	SetFocus();
	SetCapture();

	// if (!(GetAsyncKeyState(VK_MENU) & 0x8000))
	Cam_MouseDown( point.x, r.bottom - 1 - point.y, nFlags );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CCamWnd::Cam_Init()
{
	// m_Camera.draw_mode = cd_texture;
	m_Camera.origin[0] = 0.0f;
	m_Camera.origin[1] = 20.0f;
	m_Camera.origin[2] = 72.0f;
	m_Camera.color[0]  = 0.3f;
	m_Camera.color[1]  = 0.3f;
	m_Camera.color[2]  = 0.3f;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CCamWnd::Cam_BuildMatrix()
{
	float xa, ya;
	float matrix[4][4];

	xa = m_Camera.angles[PITCH] * idMath::M_DEG2RAD;
	ya = m_Camera.angles[YAW] * idMath::M_DEG2RAD;

	// Calculate forward vector
	m_Camera.forward[0] = cos( xa ) * cos( ya );
	m_Camera.forward[1] = cos( xa ) * sin( ya );
	m_Camera.forward[2] = sin( xa );

	// Calculate right vector
	m_Camera.right[0] = -sin( ya );
	m_Camera.right[1] = cos( ya );
	m_Camera.right[2] = 0;

	// Calculate up vector
	m_Camera.up = m_Camera.right.Cross( m_Camera.forward );
	m_Camera.up.Normalize();

	// This is used for selection.
	qglGetFloatv( GL_PROJECTION_MATRIX, &matrix[0][0] );

	for( int i = 0; i < 3; i++ )
	{
		m_Camera.vright[i] = matrix[i][0];
		m_Camera.vup[i]	   = matrix[i][1];
		m_Camera.vpn[i]	   = matrix[i][2];
	}

	m_Camera.vright.Normalize();
	m_Camera.vup.Normalize();
	m_Camera.vpn.Normalize();

	InitCull();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */

void CCamWnd::Cam_ChangeFloor( bool up )
{
	idEditorBrush* b;
	float		   d, bestd, current;
	idVec3		   start, dir;

	start[0] = m_Camera.origin[0];
	start[1] = m_Camera.origin[1];
	start[2] = HUGE_DISTANCE;
	dir[0] = dir[1] = 0;
	dir[2]			= -1;

	current = HUGE_DISTANCE - ( m_Camera.origin[2] - 72 );
	if( up )
	{
		bestd = 0;
	}
	else
	{
		bestd = HUGE_DISTANCE * 2;
	}

	for( b = active_brushes.next; b != &active_brushes; b = b->next )
	{
		if( !Brush_Ray( start, dir, b, &d ) )
		{
			continue;
		}

		if( up && d < current && d > bestd )
		{
			bestd = d;
		}

		if( !up && d > current && d < bestd )
		{
			bestd = d;
		}
	}

	if( bestd == 0 || bestd == HUGE_DISTANCE * 2 )
	{
		return;
	}

	m_Camera.origin[2] += current - bestd;
	Sys_UpdateWindows( W_CAMERA | W_Z_OVERLAY );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CCamWnd::Cam_PositionDrag()
{
	int x, y;
	Sys_GetCursorPos( &x, &y );
	if( x != m_ptCursor.x || y != m_ptCursor.y )
	{
		x -= m_ptCursor.x;
		VectorMA( m_Camera.origin, x, m_Camera.vright, m_Camera.origin );
		y -= m_ptCursor.y;
		m_Camera.origin[2] -= y;
		SetCursorPos( m_ptCursor.x, m_ptCursor.y );
		Sys_UpdateWindows( W_CAMERA | W_XY_OVERLAY );
	}
}

/*
================
Cam_MouseLook
================
*/
void CCamWnd::Cam_MouseLook()
{
	CPoint current;

	GetCursorPos( &current );
	if( current.x != m_ptCursor.x || current.y != m_ptCursor.y )
	{
		current.x -= m_ptCursor.x;
		current.y -= m_ptCursor.y;

		m_Camera.angles[PITCH] -= ( float )( ( float )current.y * 0.25f );
		m_Camera.angles[YAW] -= ( float )( ( float )current.x * 0.25f );

		SetCursorPos( m_ptCursor.x, m_ptCursor.y );

		Cam_BuildMatrix();
	}
}

/*
==================
Cam_MouseControl
==================
*/
void CCamWnd::Cam_MouseControl( float dtime )
{
	int	  xl, xh;
	int	  yl, yh;
	float xf, yf;
	if( g_PrefsDlg.m_nMouseButtons == 2 )
	{
		if( m_nCambuttonstate != ( MK_RBUTTON | MK_SHIFT ) )
		{
			return;
		}
	}
	else
	{
		if( m_nCambuttonstate != MK_RBUTTON )
		{
			return;
		}
	}

	xf = ( float )( m_ptButton.x - m_Camera.width / 2 ) / ( m_Camera.width / 2 );
	yf = ( float )( m_ptButton.y - m_Camera.height / 2 ) / ( m_Camera.height / 2 );

	xl = m_Camera.width / 3;
	xh = xl * 2;
	yl = m_Camera.height / 3;
	yh = yl * 2;

	// common->Printf("xf-%f yf-%f xl-%i xh-i% yl-i% yh-i%\n",xf,yf,xl,xh,yl,yh);
#if 0

	// strafe
	if( buttony < yl && ( buttonx < xl || buttonx > xh ) )
	{
		VectorMA( camera.origin, xf * dtime * g_nMoveSpeed, camera.right, camera.origin );
	}
	else
#endif
	{
		xf *= 1.0f - idMath::Fabs( yf );
		if( xf < 0.0f )
		{
			xf += 0.1f;
			if( xf > 0.0f )
			{
				xf = 0.0f;
			}
		}
		else
		{
			xf -= 0.1f;
			if( xf < 0.0f )
			{
				xf = 0.0f;
			}
		}

		VectorMA( m_Camera.origin, yf * dtime * g_PrefsDlg.m_nMoveSpeed, m_Camera.forward, m_Camera.origin );
		m_Camera.angles[YAW] += xf * -dtime * g_PrefsDlg.m_nAngleSpeed;
	}

	Cam_BuildMatrix();
	Sys_UpdateWindows( W_CAMERA | W_XY );
	g_pParentWnd->PostMessage( WM_TIMER, 0, 0 );
}

/*
==================
Cam_MouseDown
==================
*/
void CCamWnd::Cam_MouseDown( int x, int y, int buttons )
{
	idVec3 dir;
	float  f, r, u;
	int	   i;

	// calc ray direction
	u = ( float )( y - m_Camera.height / 2 ) / ( m_Camera.width / 2 );
	r = ( float )( x - m_Camera.width / 2 ) / ( m_Camera.width / 2 );
	f = 1;

	for( i = 0; i < 3; i++ )
	{
		dir[i] = m_Camera.vpn[i] * f + m_Camera.vright[i] * r + m_Camera.vup[i] * u;
	}

	dir.Normalize();

	GetCursorPos( &m_ptCursor );

	m_nCambuttonstate = buttons;
	m_ptButton.x	  = x;
	m_ptButton.y	  = y;

	//
	// LBUTTON = manipulate selection shift-LBUTTON = select middle button = grab
	// texture ctrl-middle button = set entire brush to texture ctrl-shift-middle
	// button = set single face to texture
	//
	int nMouseButton = g_PrefsDlg.m_nMouseButtons == 2 ? MK_RBUTTON : MK_MBUTTON;
	if( ( buttons == MK_LBUTTON ) || ( buttons == ( MK_LBUTTON | MK_SHIFT ) ) || ( buttons == ( MK_LBUTTON | MK_CONTROL ) ) || ( buttons == ( MK_LBUTTON | MK_CONTROL | MK_SHIFT ) ) ||
		( buttons == nMouseButton ) || ( buttons == ( nMouseButton | MK_SHIFT ) ) || ( buttons == ( nMouseButton | MK_CONTROL ) ) || ( buttons == ( nMouseButton | MK_SHIFT | MK_CONTROL ) ) )
	{
		if( g_PrefsDlg.m_nMouseButtons == 2 && ( buttons == ( MK_RBUTTON | MK_SHIFT ) ) )
		{
			Cam_MouseControl( 0.1f );
		}
		else
		{
			// something global needs to track which window is responsible for stuff
			Patch_SetView( W_CAMERA );
			Drag_Begin( x, y, buttons, m_Camera.vright, m_Camera.vup, m_Camera.origin, dir );
		}

		return;
	}

	if( buttons == MK_RBUTTON )
	{
		Cam_MouseControl( 0.1f );
		return;
	}
}

/*
==================
Cam_MouseUp
==================
*/
void CCamWnd::Cam_MouseUp( int x, int y, int buttons )
{
	m_nCambuttonstate = 0;
	Drag_MouseUp( buttons );
}

/*
==================
Cam_MouseMoved
==================
*/
void CCamWnd::Cam_MouseMoved( int x, int y, int buttons )
{
	m_nCambuttonstate = buttons;

	if( !buttons )
	{
		return;
	}

	m_ptButton.x = x;
	m_ptButton.y = y;

	if( buttons == ( MK_RBUTTON | MK_CONTROL ) )
	{
		Cam_PositionDrag();
		Sys_UpdateWindows( W_XY | W_CAMERA | W_Z );
		return;
	}
	else if( buttons == ( MK_RBUTTON | MK_CONTROL | MK_SHIFT ) )
	{
		Cam_MouseLook();
		Sys_UpdateWindows( W_XY | W_CAMERA | W_Z );
		return;
	}

	GetCursorPos( &m_ptCursor );

	if( buttons & ( MK_LBUTTON | MK_MBUTTON ) )
	{
		Drag_MouseMoved( x, y, buttons );
		Sys_UpdateWindows( W_XY | W_CAMERA | W_Z );
	}
}

/*
==================
InitCull
==================
*/
void CCamWnd::InitCull()
{
	VectorSubtract( m_Camera.vpn, m_Camera.vright, m_vCull1 );
	VectorAdd( m_Camera.vpn, m_Camera.vright, m_vCull2 );

	for( int i = 0; i < 3; i++ )
	{
		if( m_vCull1[i] > 0 )
		{
			m_nCullv1[i] = 3 + i;
		}
		else
		{
			m_nCullv1[i] = i;
		}

		if( m_vCull2[i] > 0 )
		{
			m_nCullv2[i] = 3 + i;
		}
		else
		{
			m_nCullv2[i] = i;
		}
	}
}

/*
==================
CullBrush
==================
*/
bool CCamWnd::CullBrush( idEditorBrush* b, bool cubicOnly )
{
	int	   i;
	idVec3 point;
	float  d;

	if( b->forceVisibile )
	{
		return false;
	}

	if( g_PrefsDlg.m_bCubicClipping )
	{
		float  distance = g_PrefsDlg.m_nCubicScale * 64;

		idVec3 mid;
		for( i = 0; i < 3; i++ )
		{
			mid[i] = ( b->mins[i] + ( ( b->maxs[i] - b->mins[i] ) / 2 ) );
		}

		point = mid - m_Camera.origin;
		if( point.Length() > distance )
		{
			return true;
		}
	}

	if( cubicOnly )
	{
		return false;
	}

	for( i = 0; i < 3; i++ )
	{
		point[i] = b->mins[m_nCullv1[i]] - m_Camera.origin[i];
	}

	d = DotProduct( point, m_vCull1 );
	if( d < -1 )
	{
		return true;
	}

	for( i = 0; i < 3; i++ )
	{
		point[i] = b->mins[m_nCullv2[i]] - m_Camera.origin[i];
	}

	d = DotProduct( point, m_vCull2 );
	if( d < -1 )
	{
		return true;
	}

	return false;
}

#if 0

/*
==================
DrawLightRadius
==================
*/
void CCamWnd::DrawLightRadius( idEditorBrush *pBrush )
{
	// if lighting
	int nRadius = Brush_LightRadius( pBrush );
	if( nRadius > 0 )
	{
		Brush_SetLightColor( pBrush );
		qglEnable( GL_BLEND );
		qglPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		qglBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		qglDisable( GL_BLEND );
		qglPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	}
}
#endif

/*
==================
setGLMode
==================
*/
void setGLMode( int mode )
{
	switch( mode )
	{
		case cd_wire:
			qglPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
			globalImages->BindNull();
			qglDisable( GL_BLEND );
			qglDisable( GL_DEPTH_TEST );
			qglColor3f( 1.0f, 1.0f, 1.0f );
			break;

		case cd_solid:
			qglCullFace( GL_FRONT );
			qglEnable( GL_CULL_FACE );
			qglShadeModel( GL_FLAT );
			qglPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
			globalImages->BindNull();
			qglDisable( GL_BLEND );
			qglEnable( GL_DEPTH_TEST );
			qglDepthFunc( GL_LEQUAL );
			break;

		case cd_texture:
			qglCullFace( GL_FRONT );
			qglEnable( GL_CULL_FACE );
			qglShadeModel( GL_FLAT );
			qglPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
			qglDisable( GL_BLEND );
			qglEnable( GL_DEPTH_TEST );
			qglDepthFunc( GL_LEQUAL );
			break;
	}
}

extern void glLabeledPoint( idVec4& color, idVec3& point, float size, const char* label );

/*
==================
DrawAxial
==================
*/
void		DrawAxial( face_t* selFace )
{
	if( g_bAxialMode )
	{
		idVec3 points[4];

		for( int j = 0; j < selFace->face_winding->GetNumPoints(); j++ )
		{
			glLabeledPoint( idVec4( 1, 1, 1, 1 ), ( *selFace->face_winding )[j].ToVec3(), 3, va( "%i", j ) );
		}

		ValidateAxialPoints();
		points[0] = ( *selFace->face_winding )[g_axialAnchor].ToVec3();
		VectorMA( points[0], 1, selFace->plane, points[0] );
		VectorMA( points[0], 4, selFace->plane, points[1] );
		points[3] = ( *selFace->face_winding )[g_axialDest].ToVec3();
		VectorMA( points[3], 1, selFace->plane, points[3] );
		VectorMA( points[3], 4, selFace->plane, points[2] );
		glLabeledPoint( idVec4( 1, 0, 0, 1 ), points[1], 3, "Anchor" );
		glLabeledPoint( idVec4( 1, 1, 0, 1 ), points[2], 3, "Dest" );
		qglBegin( GL_LINE_STRIP );
		qglVertex3fv( points[0].ToFloatPtr() );
		qglVertex3fv( points[1].ToFloatPtr() );
		qglVertex3fv( points[2].ToFloatPtr() );
		qglVertex3fv( points[3].ToFloatPtr() );
		qglEnd();
	}
}

/*
 =======================================================================================================================
	Cam_Draw
 =======================================================================================================================
 */

/*
==================
SetProjectionMatrix
==================
*/
void CCamWnd::SetProjectionMatrix()
{
	float xfov = 90;
	float yfov = 2 * atan( ( float )m_Camera.height / m_Camera.width ) * idMath::M_RAD2DEG;
#if 0
	float screenaspect = ( float )m_Camera.width / m_Camera.height;
	qglLoadIdentity();
	qgluPerspective( yfov, screenaspect, 2, 8192 );
#else
	float xmin, xmax, ymin, ymax;
	float width, height;
	float zNear;
	float projectionMatrix[16];

	//
	// set up projection matrix
	//
	zNear = r_znear.GetFloat();

	ymax = zNear * tan( yfov * idMath::PI / 360.0f );
	ymin = -ymax;

	xmax = zNear * tan( xfov * idMath::PI / 360.0f );
	xmin = -xmax;

	width  = xmax - xmin;
	height = ymax - ymin;

	projectionMatrix[0]	 = 2 * zNear / width;
	projectionMatrix[4]	 = 0;
	projectionMatrix[8]	 = ( xmax + xmin ) / width; // normally 0
	projectionMatrix[12] = 0;

	projectionMatrix[1]	 = 0;
	projectionMatrix[5]	 = 2 * zNear / height;
	projectionMatrix[9]	 = ( ymax + ymin ) / height; // normally 0
	projectionMatrix[13] = 0;

	// this is the far-plane-at-infinity formulation
	projectionMatrix[2]	 = 0;
	projectionMatrix[6]	 = 0;
	projectionMatrix[10] = -1;
	projectionMatrix[14] = -2 * zNear;

	projectionMatrix[3]	 = 0;
	projectionMatrix[7]	 = 0;
	projectionMatrix[11] = -1;
	projectionMatrix[15] = 0;

	qglLoadMatrixf( projectionMatrix );
#endif
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CCamWnd::DrawGrid()
{
	const float GRID_SPACING = 64.0f;
	const int	GRID_LINES	 = 100;

	// Calculate the grid bounds based on the camera position
	float		startX = floor( m_Camera.origin.x / GRID_SPACING ) * GRID_SPACING - GRID_LINES * GRID_SPACING;
	float		endX   = startX + 2 * GRID_LINES * GRID_SPACING;
	float		startY = floor( m_Camera.origin.y / GRID_SPACING ) * GRID_SPACING - GRID_LINES * GRID_SPACING;
	float		endY   = startY + 2 * GRID_LINES * GRID_SPACING;

	// Draw vertical grid lines
	qglBegin( GL_LINES );
	for( float x = startX; x <= endX; x += GRID_SPACING )
	{
		if( idMath::Fabs( x ) < 0.01f )
		{
			qglColor4f(
				g_qeglobals.d_savedinfo.colors[COLOR_GRIDMAJOR][0], g_qeglobals.d_savedinfo.colors[COLOR_GRIDMAJOR][1], g_qeglobals.d_savedinfo.colors[COLOR_GRIDMAJOR][2], 1.0f ); // Center line color
		}
		else
		{
			qglColor4f( g_qeglobals.d_savedinfo.colors[COLOR_GRIDBACK][0],
				g_qeglobals.d_savedinfo.colors[COLOR_GRIDBACK][1],
				g_qeglobals.d_savedinfo.colors[COLOR_GRIDBACK][2],
				1.0f ); // Regular grid line color
		}
		qglVertex3f( x, startY, 0 );
		qglVertex3f( x, endY, 0 );
	}
	// Draw horizontal grid lines
	for( float y = startY; y <= endY; y += GRID_SPACING )
	{
		if( idMath::Fabs( y ) < 0.01f )
		{
			qglColor4f(
				g_qeglobals.d_savedinfo.colors[COLOR_GRIDMAJOR][0], g_qeglobals.d_savedinfo.colors[COLOR_GRIDMAJOR][1], g_qeglobals.d_savedinfo.colors[COLOR_GRIDMAJOR][2], 1.0f ); // Center line color
		}
		else
		{
			qglColor4f( g_qeglobals.d_savedinfo.colors[COLOR_GRIDBACK][0],
				g_qeglobals.d_savedinfo.colors[COLOR_GRIDBACK][1],
				g_qeglobals.d_savedinfo.colors[COLOR_GRIDBACK][2],
				1.0f ); // Regular grid line color
		}
		qglVertex3f( startX, y, 0 );
		qglVertex3f( endX, y, 0 );
	}
	qglEnd();
}

/*
==================
Cam_Draw
==================
*/
void CCamWnd::Cam_Draw()
{
	idEditorBrush* brush;
	face_t*		   face;

	// float yfov;
	int			   i;

	if( !active_brushes.next )
	{
		return; // not valid yet
	}

	// set the sound origin for both simple draw and rendered mode
	// the editor uses opposite pitch convention
	idMat3 axis = idAngles( -m_Camera.angles.pitch, m_Camera.angles.yaw, m_Camera.angles.roll ).ToMat3();
	g_qeglobals.sw->PlaceListener( m_Camera.origin, axis, 0, Sys_Milliseconds(), "Undefined" );

	if( renderMode )
	{
		Cam_Render();
	}

	qglViewport( 0, 0, m_Camera.width, m_Camera.height );
	qglScissor( 0, 0, m_Camera.width, m_Camera.height );
	qglClearColor( g_qeglobals.d_savedinfo.colors[COLOR_CAMERABACK][0], g_qeglobals.d_savedinfo.colors[COLOR_CAMERABACK][1], g_qeglobals.d_savedinfo.colors[COLOR_CAMERABACK][2], 0 );

	if( !renderMode )
	{
		qglClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	}

	qglDisable( GL_LIGHTING );
	qglMatrixMode( GL_PROJECTION );

	SetProjectionMatrix();

	qglRotatef( -90, 1, 0, 0 ); // put Z going up
	qglRotatef( 90, 0, 0, 1 );	// put Z going up
	qglRotatef( m_Camera.angles[0], 0, 1, 0 );
	qglRotatef( -m_Camera.angles[1], 0, 0, 1 );
	qglTranslatef( -m_Camera.origin[0], -m_Camera.origin[1], -m_Camera.origin[2] );

	Cam_BuildMatrix();

	if( !renderMode )
	{
		DrawGrid();
	}

	// Draw Normal Opaque Brushes
	for( brush = active_brushes.next; brush != &active_brushes; brush = brush->next )
	{
		if( CullBrush( brush, false ) )
		{
			continue;
		}

		if( FilterBrush( brush ) )
		{
			continue;
		}

		if( renderMode )
		{
			if( !( entityMode && brush->owner->eclass->fixedsize ) )
			{
				continue;
			}
		}

		setGLMode( m_Camera.draw_mode );
		Brush_Draw( brush );
	}

	// qglDepthMask ( 1 ); // Ok, write now
	qglMatrixMode( GL_PROJECTION );

	qglTranslatef( g_qeglobals.d_select_translate[0], g_qeglobals.d_select_translate[1], g_qeglobals.d_select_translate[2] );

	idEditorBrush* pList = ( g_bClipMode && g_pSplitList ) ? g_pSplitList : &selected_brushes;

	if( !renderMode )
	{
		// draw normally
		for( brush = pList->next; brush != pList; brush = brush->next )
		{
			if( brush->pPatch )
			{
				continue;
			}
			setGLMode( m_Camera.draw_mode );
			Brush_Draw( brush, true );
		}
	}

	// blend on top

	setGLMode( m_Camera.draw_mode );
	qglDisable( GL_LIGHTING );
	qglColor4f( g_qeglobals.d_savedinfo.colors[COLOR_SELBRUSHES][0], g_qeglobals.d_savedinfo.colors[COLOR_SELBRUSHES][1], g_qeglobals.d_savedinfo.colors[COLOR_SELBRUSHES][2], 0.25f );
	qglEnable( GL_BLEND );
	qglPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	qglBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	globalImages->BindNull();
	for( brush = pList->next; brush != pList; brush = brush->next )
	{
		if( brush->pPatch || brush->modelHandle > 0 )
		{
			Brush_Draw( brush, true );

			// DHM - Nerve:: patch display lists/models mess with the state
			qglEnable( GL_BLEND );
			qglPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
			qglColor4f( g_qeglobals.d_savedinfo.colors[COLOR_SELBRUSHES][0], g_qeglobals.d_savedinfo.colors[COLOR_SELBRUSHES][1], g_qeglobals.d_savedinfo.colors[COLOR_SELBRUSHES][2], 0.25f );
			globalImages->BindNull();
			continue;
		}

		if( brush->owner->eclass->entityModel )
		{
			continue;
		}

		for( face = brush->brush_faces; face; face = face->next )
		{
			Face_Draw( face );
		}
	}

	int nCount = g_ptrSelectedFaces.GetSize();

	if( !renderMode )
	{
		for( i = 0; i < nCount; i++ )
		{
			face_t* selFace = reinterpret_cast<face_t*>( g_ptrSelectedFaces.GetAt( i ) );
			Face_Draw( selFace );
			DrawAxial( selFace );
		}
	}

	// non-zbuffered outline
	qglDisable( GL_BLEND );
	qglDisable( GL_DEPTH_TEST );
	qglPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

	if( renderMode )
	{
		qglColor3f( 1, 0, 0 );
		for( i = 0; i < nCount; i++ )
		{
			face_t* selFace = reinterpret_cast<face_t*>( g_ptrSelectedFaces.GetAt( i ) );
			Face_Draw( selFace );
		}
	}

	qglColor3f( 1, 1, 1 );
	for( brush = pList->next; brush != pList; brush = brush->next )
	{
		if( brush->pPatch || brush->modelHandle > 0 )
		{
			continue;
		}

		for( face = brush->brush_faces; face; face = face->next )
		{
			Face_Draw( face );
		}
	}
	// edge / vertex flags
	if( g_qeglobals.d_select_mode == sel_vertex )
	{
		qglPointSize( 4 );
		qglColor3f( 0, 1, 0 );
		qglBegin( GL_POINTS );
		for( i = 0; i < g_qeglobals.d_numpoints; i++ )
		{
			qglVertex3fv( g_qeglobals.d_points[i].ToFloatPtr() );
		}

		qglEnd();
		qglPointSize( 1 );
	}
	else if( g_qeglobals.d_select_mode == sel_edge )
	{
		float *v1, *v2;

		qglPointSize( 4 );
		qglColor3f( 0, 0, 1 );
		qglBegin( GL_POINTS );
		for( i = 0; i < g_qeglobals.d_numedges; i++ )
		{
			v1 = g_qeglobals.d_points[g_qeglobals.d_edges[i].p1].ToFloatPtr();
			v2 = g_qeglobals.d_points[g_qeglobals.d_edges[i].p2].ToFloatPtr();
			qglVertex3f( ( v1[0] + v2[0] ) * 0.5f, ( v1[1] + v2[1] ) * 0.5f, ( v1[2] + v2[2] ) * 0.5f );
		}

		qglEnd();
		qglPointSize( 1 );
	}

	g_splineList->draw( static_cast<bool>( g_qeglobals.d_select_mode == sel_addpoint || g_qeglobals.d_select_mode == sel_editpoint ) );

	if( g_qeglobals.selectObject && ( g_qeglobals.d_select_mode == sel_addpoint || g_qeglobals.d_select_mode == sel_editpoint ) )
	{
		g_qeglobals.selectObject->drawSelection();
	}

	qglEnable( GL_DEPTH_TEST );

	DrawPathLines();

	// draw pointfile
	Pointfile_Draw();

	//
	// bind back to the default texture so that we don't have problems elsewhere
	// using/modifying texture maps between contexts
	//
	globalImages->BindNull();

	qglFinish();
	QE_CheckOpenGLForErrors();

	if( !renderMode )
	{
		// clean up any deffered tri's
		R_ToggleSmpFrame();
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CCamWnd::OnSize( UINT nType, int cx, int cy )
{
	CDialogEx::OnSize( nType, cx, cy );

	CRect rect;
	GetClientRect( rect );
	m_Camera.width	= rect.right;
	m_Camera.height = rect.bottom;
	InvalidateRect( NULL, false );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CCamWnd::OnKeyUp( UINT nChar, UINT nRepCnt, UINT nFlags )
{
	g_pParentWnd->HandleKey( nChar, nRepCnt, nFlags, false );
}

//
// =======================================================================================================================
//    Timo brush primitive texture shifting, using camera view to select translations::
// =======================================================================================================================
//

/*
==================
ShiftTexture_BrushPrimit
==================
*/
void CCamWnd::ShiftTexture_BrushPrimit( face_t* f, int x, int y )
{
	/*
		idVec3	texS, texT;
		idVec3	viewX, viewY;
		int		XS, XT, YS, YT;
		int		outS, outT;
	#ifdef _DEBUG
		if (!g_qeglobals.m_bBrushPrimitMode) {
			common->Printf("Warning : unexpected call to CCamWnd::ShiftTexture_BrushPrimit with brush primitive mode disbaled\n");
			return;
		}
	#endif
		// compute face axis base
		//ComputeAxisBase(f->plane.Normal(), texS, texT);

		// compute camera view vectors
		VectorCopy(m_Camera.vup, viewY);
		VectorCopy(m_Camera.vright, viewX);

		// compute best vectors
		//ComputeBest2DVector(viewX, texS, texT, XS, XT);
		//ComputeBest2DVector(viewY, texS, texT, YS, YT);

		// check this is not a degenerate case
		if ((XS == YS) && (XT == YT))
		{
	#ifdef _DEBUG
			common->Printf("Warning : degenerate best vectors axis base in CCamWnd::ShiftTexture_BrushPrimit\n");
	#endif
			// forget it
			Select_ShiftTexture_BrushPrimit(f, x, y, false);
			return;
		}

		// compute best fitted translation in face axis base
		outS = XS * x + YS * y;
		outT = XT * x + YT * y;

		// call actual texture shifting code
		Select_ShiftTexture_BrushPrimit(f, outS, outT, false);
	*/
}

/*
==================
IsBModel
==================
*/
bool IsBModel( idEditorBrush* b )
{
	const char* v = b->owner->ValueForKey( "model" );
	if( v && *v )
	{
		const char* n = b->owner->ValueForKey( "name" );
		return ( stricmp( n, v ) == 0 );
	}
	return false;
}

/*
================
BuildEntityRenderState

Creates or updates modelDef and lightDef for an entity
================
*/
void CCamWnd::BuildEntityRenderState( idEditorEntity* ent, bool update )
{
	ent->BuildEntityRenderState( ent, update );
}

/*
==================
Tris_ToOBJ
==================
*/
void Tris_ToOBJ( const char* outFile, idTriList* tris, idMatList* mats )
{
	idFile* f = fileSystem->OpenExplicitFileWrite( outFile );
	if( f )
	{
		char out[1024];
		strcpy( out, outFile );
		StripExtension( out );

		idList<idStr*> matNames;
		int			   i, j, k;
		int			   indexBase = 1;
		idStr		   lastMaterial( "" );
		// idStr basePath = cvarSystem->GetCVarString( "fs_savepath" );
		f->Printf( "mtllib %s.mtl\n", out );
		for( i = 0; i < tris->Num(); i++ )
		{
			srfTriangles_t* tri = ( *tris )[i];
			for( j = 0; j < tri->numVerts; j++ )
			{
				f->Printf( "v %f %f %f\n", tri->verts[j].xyz.x, tri->verts[j].xyz.z, -tri->verts[j].xyz.y );
			}
			for( j = 0; j < tri->numVerts; j++ )
			{
				f->Printf( "vt %f %f\n", tri->verts[j].st.x, 1.0f - tri->verts[j].st.y );
			}
			for( j = 0; j < tri->numVerts; j++ )
			{
				f->Printf( "vn %f %f %f\n", tri->verts[j].normal.x, tri->verts[j].normal.y, tri->verts[j].normal.z );
			}

			if( stricmp( ( *mats )[i]->GetName(), lastMaterial ) )
			{
				lastMaterial = ( *mats )[i]->GetName();

				bool found = false;
				for( k = 0; k < matNames.Num(); k++ )
				{
					if( idStr::Icmp( matNames[k]->c_str(), lastMaterial.c_str() ) == 0 )
					{
						found = true;
						// f->Printf( "usemtl m%i\n", k );
						f->Printf( "usemtl %s\n", lastMaterial.c_str() );
						break;
					}
				}

				if( !found )
				{
					// f->Printf( "usemtl m%i\n", matCount++ );
					f->Printf( "usemtl %s\n", lastMaterial.c_str() );
					matNames.Append( new idStr( lastMaterial ) );
				}
			}

			for( j = 0; j < tri->numIndexes; j += 3 )
			{
				int i1, i2, i3;
				i1 = tri->indexes[j + 2] + indexBase;
				i2 = tri->indexes[j + 1] + indexBase;
				i3 = tri->indexes[j] + indexBase;
				f->Printf( "f %i/%i/%i %i/%i/%i %i/%i/%i\n", i1, i1, i1, i2, i2, i2, i3, i3, i3 );
			}

			indexBase += tri->numVerts;
		}
		fileSystem->CloseFile( f );

		strcat( out, ".mtl" );
		f = fileSystem->OpenExplicitFileWrite( out );
		if( f )
		{
			for( k = 0; k < matNames.Num(); k++ )
			{
				// This presumes the diffuse tga name matches the material name
				f->Printf(
					"newmtl %s\n\tNs 0\n\td 1\n\tillum 2\n\tKd 0 0 0 \n\tKs 0.22 0.22 0.22 \n\tKa 0 0 0 \n\tmap_Kd %s/base/%s.tga\n\n\n", matNames[k]->c_str(), "z:/d3xp", matNames[k]->c_str() );
			}
			fileSystem->CloseFile( f );
		}
	}
}

/*
==================
Select_ToCM
==================
*/
void Select_ToCM()
{
	CFileDialog dlgFile( FALSE, "lwo, ase, ma", NULL, 0, "(*.lwo)|*.lwo|(*.ase)|*.ase|(*.ma)|*.ma||", g_pParentWnd );

	if( dlgFile.DoModal() == IDOK )
	{
		idMapEntity*	mapEnt;
		idMapPrimitive* p;
		idStr			name;

		name = fileSystem->OSPathToRelativePath( dlgFile.GetPathName() );
		name.BackSlashesToSlashes();

		mapEnt = new idMapEntity();
		mapEnt->epairs.Set( "name", name.c_str() );

		for( idEditorBrush* b = selected_brushes.next; b != &selected_brushes; b = b->next )
		{
			if( b->hiddenBrush )
			{
				continue;
			}

			if( FilterBrush( b ) )
			{
				continue;
			}

			p = BrushToMapPrimitive( b, b->owner->origin );
			if( p )
			{
				mapEnt->AddPrimitive( p );
			}
		}

		collisionModelManager->WriteCollisionModelForMapEntity( mapEnt, name.c_str() );

		delete mapEnt;
	}
}

/*
=================
BuildRendererState

Builds models, lightdefs, and modeldefs for the current editor data
so it can be rendered by the game renderSystem
=================
*/
void CCamWnd::BuildRendererState()
{
	renderEntity_t	worldEntity;
	idEditorEntity* ent;
	idEditorBrush*	brush;

	FreeRendererState();

	// the renderWorld holds all the references and defs
	g_qeglobals.rw->InitFromMap( NULL );

	// create the raw model for all the brushes
	int numBrushes	= 0;
	int numSurfaces = 0;

	// the renderModel for the world holds all the geometry that isn't in an entity
	worldModel = renderModelManager->AllocModel();
	worldModel->InitEmpty( "EditorWorldModel" );

	for( idEditorBrush* brushList = &active_brushes; brushList; brushList = ( brushList == &active_brushes ) ? &selected_brushes : NULL )
	{
		for( brush = brushList->next; brush != brushList; brush = brush->next )
		{
			if( brush->hiddenBrush )
			{
				continue;
			}

			if( FilterBrush( brush ) )
			{
				continue;
			}

			idTriList tris( 1024 );
			idMatList mats( 1024 );

			if( !IsBModel( brush ) )
			{
				numSurfaces += Brush_ToTris( brush, &tris, &mats, false, false );
			}

			// add the surfaces to the renderModel
			modelSurface_t surf;
			for( int i = 0; i < tris.Num(); i++ )
			{
				surf.geometry = tris[i];
				surf.shader	  = mats[i];
				worldModel->AddSurface( surf );
			}

			numBrushes++;
		}
	}

	// bound and clean the triangles
	worldModel->FinishSurfaces();

	// the worldEntity just has the handle for the worldModel
	memset( &worldEntity, 0, sizeof( worldEntity ) );
	worldEntity.hModel		   = worldModel;
	worldEntity.axis		   = mat3_default;
	worldEntity.shaderParms[0] = 1;
	worldEntity.shaderParms[1] = 1;
	worldEntity.shaderParms[2] = 1;
	worldEntity.shaderParms[3] = 1;

	worldModelDef = g_qeglobals.rw->AddEntityDef( &worldEntity );

	// create the light and model entities exactly the way the game code would
	for( ent = entities.next; ent != &entities; ent = ent->next )
	{
		if( ent->brushes.onext == &ent->brushes )
		{
			continue;
		}

		if( CullBrush( ent->brushes.onext, true ) )
		{
			continue;
		}

		if( Map_IsBrushFiltered( ent->brushes.onext ) )
		{
			continue;
		}

		BuildEntityRenderState( ent, false );
	}

	// common->Printf("Render data used %d brushes\n", numBrushes);
	worldDirty = false;
	UpdateCaption();
}

/*
==================
UpdateRenderEntities

  Creates a new entity state list
  returns true if a repaint is needed
==================
*/
bool CCamWnd::UpdateRenderEntities()
{
	bool ret = false;
	for( idEditorEntity* ent = entities.next; ent != &entities; ent = ent->next )
	{
		BuildEntityRenderState( ent, ( ent->lightDef != -1 || ent->modelDef != -1 || ent->soundEmitter ) ? true : false );
		if( ret == false && ent->modelDef || ent->lightDef )
		{
			ret = true;
		}
	}
	return ret;
}

/*
==================
FreeRendererState

  Frees the render state data
==================
*/
void CCamWnd::FreeRendererState()
{
	for( idEditorEntity* ent = entities.next; ent != &entities; ent = ent->next )
	{
		if( ent->lightDef >= 0 )
		{
			g_qeglobals.rw->FreeLightDef( ent->lightDef );
			ent->lightDef = -1;
		}

		if( ent->modelDef >= 0 )
		{
			renderEntity_t* refent = const_cast<renderEntity_t*>( g_qeglobals.rw->GetRenderEntity( ent->modelDef ) );
			if( refent )
			{
				if( refent->callbackData )
				{
					Mem_Free( refent->callbackData );
					refent->callbackData = NULL;
				}
				if( refent->joints )
				{
					Mem_Free16( refent->joints );
					refent->joints = NULL;
				}
			}
			g_qeglobals.rw->FreeEntityDef( ent->modelDef );
			ent->modelDef = -1;
		}
	}

	if( worldModel )
	{
		renderModelManager->FreeModel( worldModel );
		worldModel = NULL;
	}
}

/*
==================
UpdateCaption

  updates the caption based on rendermode and whether the render mode needs updated
==================
*/
void CCamWnd::UpdateCaption()
{
	idStr strCaption;

	if( worldDirty )
	{
		strCaption = "*";
	}
	// FIXME:
	strCaption += ( renderMode ) ? "RENDER" : "CAM";
	if( renderMode )
	{
		strCaption += ( rebuildMode ) ? " (Realtime)" : "";
		strCaption += ( entityMode ) ? " +lights" : "";
		strCaption += ( selectMode ) ? " +selected" : "";
		strCaption += ( animationMode ) ? " +anim" : "";
	}
	strCaption += ( soundMode ) ? " +snd" : "";
	SetWindowText( strCaption );
}

/*
==================
ToggleRenderMode
==================
*/
void CCamWnd::ToggleRenderMode()
{
	renderMode ^= 1;
	UpdateCaption();
}

/*
==================
ToggleRebuildMode
==================
*/
void CCamWnd::ToggleRebuildMode()
{
	rebuildMode ^= 1;
	UpdateCaption();
}

/*
==================
ToggleEntityMode
==================
*/
void CCamWnd::ToggleEntityMode()
{
	entityMode ^= 1;
	UpdateCaption();
}

/*
==================
ToggleAnimationMode
==================
*/
void CCamWnd::ToggleAnimationMode()
{
	animationMode ^= 1;
	if( animationMode )
	{
		SetTimer( 0, 10, NULL );
	}
	else
	{
		KillTimer( 0 );
	}
	UpdateCaption();
}

/*
==================
ToggleSoundMode
==================
*/
void CCamWnd::ToggleSoundMode()
{
	soundMode ^= 1;

	UpdateCaption();

	for( idEditorEntity* ent = entities.next; ent != &entities; ent = ent->next )
	{
		ent->UpdateSoundEmitter();
	}
}

/*
==================
ToggleSelectMode
==================
*/
void CCamWnd::ToggleSelectMode()
{
	selectMode ^= 1;
	UpdateCaption();
}

/*
==================
MarkWorldDirty
==================
*/
void CCamWnd::MarkWorldDirty()
{
	worldDirty = true;
	UpdateCaption();
}

extern void glBox( idVec4& color, idVec3& point, float size );

/*
==================
DrawEntityData

  Draws entity data ( experimental )
==================
*/
void		CCamWnd::DrawEntityData()
{
	qglMatrixMode( GL_MODELVIEW );
	qglLoadIdentity();
	qglMatrixMode( GL_PROJECTION );
	qglLoadIdentity();

	SetProjectionMatrix();

	qglRotatef( -90, 1, 0, 0 ); // put Z going up
	qglRotatef( 90, 0, 0, 1 );	// put Z going up
	qglRotatef( m_Camera.angles[0], 0, 1, 0 );
	qglRotatef( -m_Camera.angles[1], 0, 0, 1 );
	qglTranslatef( -m_Camera.origin[0], -m_Camera.origin[1], -m_Camera.origin[2] );

	Cam_BuildMatrix();

	if( !( entityMode || selectMode ) )
	{
		return;
	}

	qglDisable( GL_BLEND );
	qglDisable( GL_DEPTH_TEST );
	qglPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
	globalImages->BindNull();
	idVec3 color( 0, 1, 0 );
	qglColor3fv( color.ToFloatPtr() );

	idEditorBrush* brushList = &active_brushes;
	int			   pass		 = 0;
	while( brushList )
	{
		for( idEditorBrush* brush = brushList->next; brush != brushList; brush = brush->next )
		{
			if( CullBrush( brush, true ) )
			{
				continue;
			}

			if( FilterBrush( brush ) )
			{
				continue;
			}

			if( ( pass == 1 && selectMode ) || ( entityMode && pass == 0 && brush->owner->lightDef >= 0 ) )
			{
				Brush_DrawXY( brush, ( ViewType )0, true, true );
			}
		}
		brushList = ( brushList == &active_brushes ) ? &selected_brushes : NULL;
		color.x	  = 1;
		color.y	  = 0;
		pass++;
		qglColor3fv( color.ToFloatPtr() );
	}
}

/*
==================
Cam_Render

	This used the renderSystem to
	draw a fully lit view of the world
==================
 */
void CCamWnd::Cam_Render()
{
	renderView_t refdef;
	CPaintDC	 dc( this ); // device context for painting

	if( !active_brushes.next )
	{
		return; // not valid yet
	}

	// DG: from SteelStorm2
	// Jmarshal23 recommended to disable this to fix lighting render in the Cam window
	/* if (!qwglMakeCurrent(dc.m_hDC, win32.hGLRC)) {
		common->Printf("ERROR: qwglMakeCurrent failed..\n ");
		common->Printf("Please restart " EDITOR_WINDOWTEXT " if the camera view is not working\n");
		return;
	} */

	// save the editor state
	// qglPushAttrib( GL_ALL_ATTRIB_BITS );
	qglClearColor( 0.1f, 0.1f, 0.1f, 0.0f );
	qglScissor( 0, 0, m_Camera.width, m_Camera.height );
	qglClear( GL_COLOR_BUFFER_BIT );

	//	qwglSwapBuffers(dc.m_hDC);

	// create the model, using explicit normals
	if( rebuildMode && worldDirty )
	{
		BuildRendererState();
	}

	// render it
	renderSystem->BeginFrame( m_Camera.width, m_Camera.height );

	memset( &refdef, 0, sizeof( refdef ) );
	refdef.vieworg = m_Camera.origin;

	// the editor uses opposite pitch convention
	refdef.viewaxis = idAngles( -m_Camera.angles.pitch, m_Camera.angles.yaw, m_Camera.angles.roll ).ToMat3();

	refdef.width  = SCREEN_WIDTH;
	refdef.height = SCREEN_HEIGHT;
	refdef.fov_x  = 90;
	refdef.fov_y  = 2 * atan( ( float )m_Camera.height / m_Camera.width ) * idMath::M_RAD2DEG;

	// only set in animation mode to give a consistent look
	if( animationMode )
	{
		refdef.time = eventLoop->Milliseconds();
	}

	g_qeglobals.rw->RenderScene( &refdef );

	int frontEnd, backEnd;

	renderSystem->EndFrame( &frontEnd, &backEnd );
	// common->Printf( "front:%i back:%i\n", frontEnd, backEnd );

	// qglPopAttrib();
	// DrawEntityData();

	// qwglSwapBuffers(dc.m_hDC);
	//  get back to the editor state
	qglMatrixMode( GL_MODELVIEW );
	qglLoadIdentity();
	Cam_BuildMatrix();
}

void CCamWnd::OnTimer( UINT_PTR nIDEvent )
{
	if( ( renderMode && animationMode ) || nIDEvent == 1 )
	{
		Sys_UpdateWindows( W_CAMERA );
	}
	if( nIDEvent == 1 )
	{
		KillTimer( 1 );
	}

	if( !animationMode )
	{
		KillTimer( 0 );
	}
}

/*
==================
CCamWnd::UpdateCameraView
==================
*/
void CCamWnd::UpdateCameraView()
{
	if( QE_SingleBrush( true, true ) )
	{
		idEditorBrush* b = selected_brushes.next;
		if( b->owner->eclass->nShowFlags & ECLASS_CAMERAVIEW )
		{
			// find the entity that targets this
			const char*		name = b->owner->ValueForKey( "name" );
			idEditorEntity* ent	 = FindEntity( "target", name );
			if( ent )
			{
				if( !saveValid )
				{
					saveOrg	  = m_Camera.origin;
					saveAng	  = m_Camera.angles;
					saveValid = true;
				}
				idVec3 v = b->owner->origin - ent->origin;
				v.Normalize();
				idAngles ang = v.ToMat3().ToAngles();
				ang.pitch	 = -ang.pitch;
				ang.roll	 = 0.0f;
				SetView( ent->origin, ang );
				Cam_BuildMatrix();
				Sys_UpdateWindows( W_CAMERA );
				return;
			}
		}
	}
	if( saveValid )
	{
		SetView( saveOrg, saveAng );
		Cam_BuildMatrix();
		Sys_UpdateWindows( W_CAMERA );
		saveValid = false;
	}
}

/*
==================
CCamWnd::EnableMouseLook
==================
*/
void CCamWnd::EnableMouseLook( bool enable )
{
	m_bMouseLook = enable;
	if( enable )
	{
		GetCursorPos( &m_LastMousePos );
		ShowCursor( FALSE ); // Hide the cursor for mouse look
	}
	else
	{
		ShowCursor( TRUE ); // Show the cursor again
	}
}

/*
==================
CCamWnd::UpdateCameraOrientation
==================
*/
void CCamWnd::UpdateCameraOrientation( float dx, float dy )
{
	m_Camera.angles[YAW] += dx * -m_MouseSensitivity;
	m_Camera.angles[PITCH] += dy * m_MouseSensitivity;
	if( m_Camera.angles[PITCH] > 89.0f )
	{
		m_Camera.angles[PITCH] = 89.0f;
	}
	if( m_Camera.angles[PITCH] < -89.0f )
	{
		m_Camera.angles[PITCH] = -89.0f;
	}
	Cam_BuildMatrix();
	Sys_UpdateWindows( W_CAMERA );
}

/*
==================
CCamWnd::UpdateCameraPosition
==================
*/
void CCamWnd::UpdateCameraPosition( float dx, float dy, float dz )
{
	idVec3 forward = m_Camera.forward;
	idVec3 right   = m_Camera.right;
	idVec3 up	   = idVec3( 0, 0, 1 );

	m_Camera.origin += forward * dx + right * dy + up * dz;
	Sys_UpdateWindows( W_CAMERA );
}