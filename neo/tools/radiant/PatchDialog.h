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

// CPatchDialog dialog

class CPatchDialog : public CDialogEx {
  patchMesh_t *m_Patch;

public:
	void UpdateInfo();
	void SetPatchInfo();
	void GetPatchInfo();
	CPatchDialog( CWnd *pParent = NULL );   // standard constructor
	void UpdateSpinners( bool bUp, int nID );

	enum { IDD = IDD_DIALOG_PATCH };
	CSpinButtonCtrl	m_wndVShift;
	CSpinButtonCtrl	m_wndVScale;
	CSpinButtonCtrl	m_wndRotate;
	CSpinButtonCtrl	m_wndHShift;
	CSpinButtonCtrl	m_wndHScale;
	CComboBox	m_wndType;
	CComboBox	m_wndRows;
	CComboBox	m_wndCols;
	CString	m_strName;
	float	m_fS;
	float	m_fT;
	float	m_fX;
	float	m_fY;
	float	m_fZ;
	float	m_fHScale;
	float	m_fHShift;
	float	m_fRotate;
	float	m_fVScale;
	float	m_fVShift;

protected:
	virtual void DoDataExchange( CDataExchange *pDX );    // DDX/DDV support

protected:
	void UpdateRowColInfo();

	afx_msg void OnBtnPatchdetails();
	afx_msg void OnBtnPatchfit();
	afx_msg void OnBtnPatchnatural();
	afx_msg void OnBtnPatchreset();
	afx_msg void OnSelchangeComboCol();
	afx_msg void OnSelchangeComboRow();
	afx_msg void OnSelchangeComboType();
	virtual void OnOK();
	afx_msg void OnDeltaposSpin( NMHDR *pNMHDR, LRESULT *pResult );
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnApply();

	DECLARE_MESSAGE_MAP()
};