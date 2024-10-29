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

#pragma once

// CFindTextureDlg dialog

void HideInfoDialog();
void ShowInfoDialog( const char *pText );

class CFindTextureDlg : public CDialogEx {

public:
    CFindTextureDlg( CWnd *pParent = nullptr );   // standard constructor

    enum { IDD = IDD_DIALOG_FINDREPLACE };

    static  void updateTextures( const char *p );
    static void show();
    static bool isOpen();
    static void setFindStr( const char *p );
    static void setReplaceStr( const char *p );

protected:
    virtual void DoDataExchange( CDataExchange *pDX ) override;    // DDX/DDV support

    DECLARE_MESSAGE_MAP()

private:
    BOOL m_bSelectedOnly;
    CString m_strFind;
    CString m_strReplace;
    BOOL m_bForce;
    BOOL m_bLive;

    afx_msg void OnBtnApply();
    afx_msg void OnOK();
    afx_msg  void OnCancel();
    afx_msg void OnSetfocusEditFind();
    afx_msg void OnSetfocusEditReplace();
};