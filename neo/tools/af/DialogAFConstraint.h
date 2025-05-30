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

class DialogAFConstraintFixed;
class DialogAFConstraintBallAndSocket;
class DialogAFConstraintUniversal;
class DialogAFConstraintHinge;
class DialogAFConstraintSlider;
class DialogAFConstraintSpring;

// DialogAFConstraint dialog

class DialogAFConstraint : public CDialog
{

	DECLARE_DYNAMIC( DialogAFConstraint )

public:
	DialogAFConstraint( CWnd* pParent = NULL );   // standard constructor
	virtual				~DialogAFConstraint();
	void				LoadFile( idDeclAF* af );
	void				SaveFile();
	void				LoadConstraint( const char* name );
	void				SaveConstraint();
	void				UpdateFile();

	enum				{ IDD = IDD_DIALOG_AF_CONSTRAINT };

protected:
	virtual BOOL		OnInitDialog();
	virtual void		DoDataExchange( CDataExchange* pDX );    // DDX/DDV support
	virtual INT_PTR		OnToolHitTest( CPoint point, TOOLINFO* pTI ) const;
	afx_msg BOOL		OnToolTipNotify( UINT id, NMHDR* pNMHDR, LRESULT* pResult );
	afx_msg void		OnShowWindow( BOOL bShow, UINT nStatus );
	afx_msg void		OnCbnSelchangeComboConstraints();
	afx_msg void		OnBnClickedButtonNewconstraint();
	afx_msg void		OnBnClickedButtonRenameconstraint();
	afx_msg void		OnBnClickedButtonDeleteconstraint();
	afx_msg void		OnCbnSelchangeComboConstraintType();
	afx_msg void		OnCbnSelchangeComboConstraintBody1();
	afx_msg void		OnCbnSelchangeComboConstraintBody2();
	afx_msg void		OnEnChangeEditConstraintFriction();
	afx_msg void		OnDeltaposSpinConstraintFriction( NMHDR* pNMHDR, LRESULT* pResult );

	DECLARE_MESSAGE_MAP()

private:
	idDeclAF* 			file;
	idDeclAF_Constraint* constraint;
	CDialog* 			constraintDlg;
	DialogAFConstraintFixed* fixedDlg;
	DialogAFConstraintBallAndSocket* ballAndSocketDlg;
	DialogAFConstraintUniversal* universalDlg;
	DialogAFConstraintHinge* hingeDlg;
	DialogAFConstraintSlider* sliderDlg;
	DialogAFConstraintSpring* springDlg;

	//{{AFX_DATA(DialogAFConstraint)
	CComboBox			m_comboConstraintList;			// list with constraints
	CComboBox			m_comboConstraintType;
	CComboBox			m_comboBody1List;
	CComboBox			m_comboBody2List;
	float				m_friction;
	//}}AFX_DATA

	static toolTip_t	toolTips[];

private:
	void				InitConstraintList();
	void				InitConstraintTypeDlg();
	void				InitBodyLists();
	void				InitNewRenameDeleteButtons();
};
