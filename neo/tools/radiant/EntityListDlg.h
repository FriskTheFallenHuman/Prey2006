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

#pragma once

// CEntityListDlg dialog

class CEntityListDlg : public CDialogEx
{
public:
	CEntityListDlg( CWnd* pParent = NULL ); // standard constructor
	void		UpdateList();
	static void ShowDialog();

	enum
	{
		IDD = IDD_DLG_ENTITYLIST
	};
	CListCtrl m_lstEntity;

protected:
	virtual void DoDataExchange( CDataExchange* pDX ); // DDX/DDV support

protected:
	afx_msg void OnSelect();
	afx_msg void OnClose();
	virtual void OnCancel();
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand( UINT nID, LPARAM lParam );

	DECLARE_MESSAGE_MAP()

public:
	CListBox	 listEntities;
	afx_msg void OnLbnSelchangeListEntities();
	afx_msg void OnLbnDblclkListEntities();
};