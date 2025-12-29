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
#include "SplashScreen.h"

CSplashScreen* CSplashScreen::c_pSplashWnd = NULL;

CSplashScreen::CSplashScreen()
{
}

CSplashScreen::~CSplashScreen()
{
}

BEGIN_MESSAGE_MAP( CSplashScreen, CWnd )
ON_WM_PAINT()
ON_WM_TIMER()
END_MESSAGE_MAP()

void CSplashScreen::ShowSplashScreen( CWnd* pParentWnd )
{
	if( c_pSplashWnd == NULL )
	{
		c_pSplashWnd = new CSplashScreen;
		if( !c_pSplashWnd->Create( pParentWnd ) )
		{
			delete c_pSplashWnd;
		}
	}
}

void CSplashScreen::HideSplashScreen()
{
	if( c_pSplashWnd != NULL )
	{
		c_pSplashWnd->Hide();
		c_pSplashWnd = NULL;
	}
}

BOOL CSplashScreen::Create( CWnd* pParentWnd )
{
	if( !m_bitmap.LoadBitmap( IDB_BITMAP_LOGO ) )
	{
		return FALSE;
	}

	BITMAP bm;
	m_bitmap.GetBitmap( &bm );

	CRect rect( 0, 0, bm.bmWidth, bm.bmHeight );
	DWORD dwStyle = WS_POPUP | WS_VISIBLE;

	if( !CWnd::CreateEx( 0, AfxRegisterWndClass( 0, AfxGetApp()->LoadStandardCursor( IDC_ARROW ) ), NULL, dwStyle, rect, pParentWnd, 0 ) )
	{
		return FALSE;
	}

	// Set the splash screen always in the foreground
	SetWindowPos( &CWnd::wndTopMost, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );

	CenterWindow();

	//	SetTimer(1, 3000, NULL);

	UpdateWindow();

	return TRUE;
}

void CSplashScreen::Hide()
{
	KillTimer( 1 );
	DestroyWindow();
}

void CSplashScreen::PostNcDestroy()
{
	delete this;
}

void CSplashScreen::OnPaint()
{
	CPaintDC dc( this );
	DrawSplash( &dc );
}

void CSplashScreen::DrawSplash( CDC* pDC )
{
	CDC dcImage;
	dcImage.CreateCompatibleDC( pDC );

	CBitmap* pOldBitmap = dcImage.SelectObject( &m_bitmap );

	BITMAP	 bm;
	m_bitmap.GetBitmap( &bm );

	pDC->BitBlt( 0, 0, bm.bmWidth, bm.bmHeight, &dcImage, 0, 0, SRCCOPY );

	dcImage.SelectObject( pOldBitmap );
}

void CSplashScreen::OnTimer( UINT_PTR nIDEvent )
{
	// Hide();
}