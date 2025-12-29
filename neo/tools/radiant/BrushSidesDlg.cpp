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
#include "BrushSidesDlg.h"

#ifdef _DEBUG
	#define new DEBUG_NEW
#endif

IMPLEMENT_DYNAMIC( CBrushSidesDlg, CDialogEx )

CBrushSidesDlg::CBrushSidesDlg( bool bDoCone, bool bDoSphere, CWnd* pParent ) :
	CDialogEx( IDD_SIDES, pParent ),
	m_bDoCone( bDoCone ),
	m_bDoSphere( bDoSphere )
{
}

CBrushSidesDlg::~CBrushSidesDlg()
{
}

void CBrushSidesDlg::DoDataExchange( CDataExchange* pDX )
{
	CDialogEx::DoDataExchange( pDX );
	DDX_Control( pDX, IDC_SIDES, m_editSides );
}

BEGIN_MESSAGE_MAP( CBrushSidesDlg, CDialogEx )
ON_BN_CLICKED( IDOK, OnOK )
ON_BN_CLICKED( IDCANCEL, OnCancel )
END_MESSAGE_MAP()

BOOL CBrushSidesDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_editSides.SetFocus();

	return FALSE; // return TRUE unless you set the focus to a control
}

void CBrushSidesDlg::OnOK()
{
	CString str;
	m_editSides.GetWindowText( str );

	if( m_bDoCone )
	{
		Brush_MakeSidedCone( _ttoi( str ) );
	}
	else if( m_bDoSphere )
	{
		Brush_MakeSidedSphere( _ttoi( str ) );
	}
	else
	{
		Brush_MakeSided( _ttoi( str ) );
	}

	CDialogEx::OnOK();
}

void CBrushSidesDlg::OnCancel()
{
	CDialogEx::OnCancel();
}

void DoSides( bool bCone, bool bSphere, bool bTorus )
{
	CBrushSidesDlg dlg( bCone, bSphere );
	dlg.DoModal();
}