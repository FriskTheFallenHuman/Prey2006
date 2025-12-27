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

#define PIT_COMBO		0 // PIT = property item type
#define PIT_EDIT		1
#define PIT_COLOR		2
#define PIT_FONT		3
#define PIT_FILE		4
#define PIT_SCRIPT		5
#define PIT_MODEL		6
#define PIT_SOUND		7
#define PIT_GUI			8
#define PIT_MATERIAL	9
#define PIT_VAR			10

#define IDC_PROPCMBBOX	712
#define IDC_PROPEDITBOX 713
#define IDC_PROPBTNCTRL 714

// CPropertyList Items

class CPropertyItem
{
public:
	CString m_propName;
	CString m_curValue;
	int		m_nItemType;
	CString m_cmbItems;
	int		data;

public:
	CPropertyItem( CString propName, CString curValue, int nItemType, CString cmbItems )
	{
		m_propName	= propName;
		m_curValue	= curValue;
		m_nItemType = nItemType;
		m_cmbItems	= cmbItems;
		data		= -1;
	}
	void SetData( int d ) { data = d; }
};

// CPropertyList window

class CPropertyList : public CListBox
{
public:
	CPropertyList();

public:
	int			 AddItem( CString txt );
	int			 AddPropItem( CPropertyItem* pItem );
	void		 ResetContent();
	CEdit*		 GetEditBox() { return &m_editBox; }
	void		 SetUpdateInspectors( bool b ) { updateInspectors = b; }
	void		 SetDivider( int div ) { m_nDivider = div; }
	afx_msg void OnKillfocusEditBox();
	afx_msg void OnChangeEditBox();

public:
	virtual void MeasureItem( LPMEASUREITEMSTRUCT lpMeasureItemStruct );
	virtual void DrawItem( LPDRAWITEMSTRUCT lpDrawItemStruct );
	afx_msg void OnSelchange();

protected:
	virtual BOOL PreCreateWindow( CREATESTRUCT& cs );
	virtual void PreSubclassWindow();

public:
	virtual ~CPropertyList();

protected:
	afx_msg int	 OnCreate( LPCREATESTRUCT lpCreateStruct );
	afx_msg void OnLButtonUp( UINT nFlags, CPoint point );
	afx_msg void OnKillFocus( CWnd* pNewWnd );
	afx_msg void OnLButtonDown( UINT nFlags, CPoint point );
	afx_msg void OnMouseMove( UINT nFlags, CPoint point );
	afx_msg void OnVScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar );
	afx_msg void OnKillfocusCmbBox();
	afx_msg void OnSelchangeCmbBox();
	afx_msg void OnButton();

	DECLARE_MESSAGE_MAP()

	void		   InvertLine( CDC* pDC, CPoint ptFrom, CPoint ptTo );
	void		   DisplayButton( CRect region );

	CComboBox	   m_cmbBox;
	CEdit		   m_editBox;
	CButton		   m_btnCtrl;
	CFont		   m_SShellDlg8Font;

	int			   m_curSel, m_prevSel;
	int			   m_nDivider;
	int			   m_nDivTop;
	int			   m_nDivBtm;
	int			   m_nOldDivX;
	int			   m_nLastBox;
	BOOL		   m_bTracking;
	BOOL		   m_bDivIsSet;
	HCURSOR		   m_hCursorArrow;
	HCURSOR		   m_hCursorSize;
	CPropertyItem* measureItem;
	bool		   updateInspectors;
};