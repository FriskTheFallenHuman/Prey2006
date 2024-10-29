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

#include "AboutBoxDlg.h"

CAboutDlg::CAboutDlg( UINT nIDTemplate, CWnd *pParent /*=nullptr*/ )
	: CDialog( nIDTemplate, pParent ) {
}

void CAboutDlg::DoDataExchange( CDataExchange *pDX ) {
	CDialog::DoDataExchange( pDX );
}

BEGIN_MESSAGE_MAP( CAboutDlg, CDialog )
	ON_COMMAND( IDOK, &CAboutDlg::OnOK )
END_MESSAGE_MAP()

void CAboutDlg::OnOK( void ) {
	EndDialog( IDOK );
}

void CAboutDlg::SetDialogTitle( const CString &title ) {
	m_strTitle = title;
}

BOOL CAboutDlg::OnInitDialog() {
	CDialog::OnInitDialog();

	if ( !m_strTitle.IsEmpty() ) {
		SetWindowText( m_strTitle );
	}

	CString buffer;
	buffer.Format( "Renderer:\t%s", qglGetString( GL_RENDERER ) );
	SetDlgItemText( IDC_ABOUT_GLRENDERER, buffer);

	buffer.Format( "Version:\t\t%s", qglGetString( GL_VERSION ) );
	SetDlgItemText( IDC_ABOUT_GLVERSION, buffer );

	buffer.Format( "Vendor:\t\t%s", qglGetString( GL_VENDOR ) );
	SetDlgItemText( IDC_ABOUT_GLVENDOR, buffer );

	const GLubyte *extensions = qglGetString( GL_EXTENSIONS );
	if ( extensions ) {
		CString extStr = (char *)extensions;
		CListBox *pListBox = (CListBox *)GetDlgItem( IDC_ABOUT_GLEXTENSIONS );

		int start = 0;
		int end;
		while ( ( end = extStr.Find( ' ', start ) ) != -1 ) {
			pListBox->AddString( extStr.Mid( start, end - start ) );
			start = end + 1;
		}
		pListBox->AddString( extStr.Mid( start ) );
	} else {
		SetDlgItemText( IDC_ABOUT_GLEXTENSIONS, "No extensions found." );
	}

	return TRUE;
}