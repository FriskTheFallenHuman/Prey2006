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

#include "qe3.h"
#include "Radiant.h"
#include "PatchDensityDlg.h"

#ifdef _DEBUG
	#define new DEBUG_NEW
#endif

// CPatchDensityDlg dialog

CPatchDensityDlg::CPatchDensityDlg( CWnd* pParent )
	: CDialogEx( CPatchDensityDlg::IDD, pParent )
{
}


void CPatchDensityDlg::DoDataExchange( CDataExchange* pDX )
{
	CDialogEx::DoDataExchange( pDX );
	DDX_Control( pDX, IDC_COMBO_WIDTH, m_wndWidth );
	DDX_Control( pDX, IDC_COMBO_HEIGHT, m_wndHeight );
}


BEGIN_MESSAGE_MAP( CPatchDensityDlg, CDialogEx )
END_MESSAGE_MAP()

// CPatchDensityDlg message handlers

int g_nXLat[] = {3, 5, 7, 9, 11, 13, 15};

void CPatchDensityDlg::OnOK()
{
	int nWidth = m_wndWidth.GetCurSel();
	int nHeight = m_wndHeight.GetCurSel();

	if( nWidth >= 0 && nWidth <= 6 && nHeight >= 0 && nHeight <= 6 )
	{
		Patch_GenericMesh( g_nXLat[nWidth], g_nXLat[nHeight], ( int )g_pParentWnd->ActiveXY()->GetViewType() );
		Sys_UpdateWindows( W_ALL );
	}

	CDialogEx::OnOK();
}

BOOL CPatchDensityDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_wndWidth.SetCurSel( 0 );
	m_wndHeight.SetCurSel( 0 );
	return TRUE;
}
