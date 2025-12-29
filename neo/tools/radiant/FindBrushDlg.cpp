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
#include "FindBrushDlg.h"
#include "MainFrm.h"

#ifdef _DEBUG
	#define new DEBUG_NEW
#endif

/*
=================
SelectBrush
=================
*/
void SelectBrush( int entitynum, int brushnum )
{
	idEditorEntity* e;
	idEditorBrush*	b;
	int				i;

	if( entitynum == 0 )
	{
		e = world_entity;
	}
	else
	{
		e = entities.next;
		while( --entitynum )
		{
			e = e->next;
			if( e == &entities )
			{
				Sys_Status( "No such entity.", 0 );
				return;
			}
		}
	}

	b = e->brushes.onext;
	if( b == &e->brushes )
	{
		Sys_Status( "No such brush.", 0 );
		return;
	}
	while( brushnum-- )
	{
		b = b->onext;
		if( b == &e->brushes )
		{
			Sys_Status( "No such brush.", 0 );
			return;
		}
	}

	Brush_RemoveFromList( b );
	Brush_AddToList( b, &selected_brushes );

	Sys_UpdateWindows( W_ALL );
	for( i = 0; i < 3; i++ )
	{
		if( g_pParentWnd->GetXYWnd() )
		{
			g_pParentWnd->GetXYWnd()->GetOrigin()[i] = ( b->mins[i] + b->maxs[i] ) / 2;
		}

		if( g_pParentWnd->GetXZWnd() )
		{
			g_pParentWnd->GetXZWnd()->GetOrigin()[i] = ( b->mins[i] + b->maxs[i] ) / 2;
		}

		if( g_pParentWnd->GetYZWnd() )
		{
			g_pParentWnd->GetYZWnd()->GetOrigin()[i] = ( b->mins[i] + b->maxs[i] ) / 2;
		}
	}

	Sys_Status( "Selected.", 0 );
}

/*
=================
GetSelectionIndex
=================
*/
void GetSelectionIndex( int* ent, int* brush )
{
	idEditorBrush * b, *b2;
	idEditorEntity* entity;

	*ent = *brush = 0;

	b = selected_brushes.next;
	if( b == &selected_brushes )
	{
		return;
	}

	// find entity
	if( b->owner != world_entity )
	{
		( *ent )++;
		for( entity = entities.next; entity != &entities; entity = entity->next, ( *ent )++ )
			;
	}

	// find brush
	for( b2 = b->owner->brushes.onext; b2 != b && b2 != &b->owner->brushes; b2 = b2->onext, ( *brush )++ )
		;
}

IMPLEMENT_DYNAMIC( CFindBrushDlg, CDialogEx )

CFindBrushDlg::CFindBrushDlg( CWnd* pParent ) :
	CDialogEx( IDD_FINDBRUSH, pParent )
{
}

CFindBrushDlg::~CFindBrushDlg( void )
{
}

void CFindBrushDlg::DoDataExchange( CDataExchange* pDX )
{
	CDialogEx::DoDataExchange( pDX );
	DDX_Control( pDX, IDC_FIND_ENTITY, m_editEntity );
	DDX_Control( pDX, IDC_FIND_BRUSH, m_editBrush );
}

BEGIN_MESSAGE_MAP( CFindBrushDlg, CDialogEx )
ON_BN_CLICKED( IDOK, &CFindBrushDlg::OnBnClickedOk )
ON_BN_CLICKED( IDCANCEL, &CFindBrushDlg::OnBnClickedCancel )
END_MESSAGE_MAP()

BOOL CFindBrushDlg::OnInitDialog( void )
{
	CDialogEx::OnInitDialog();

	int ent, brush;
	GetSelectionIndex( &ent, &brush );

	CString entStr;
	entStr.Format( _T( "%d" ), ent );
	m_editEntity.SetWindowText( entStr );

	CString brushStr;
	brushStr.Format( _T( "%d" ), brush );
	m_editBrush.SetWindowText( brushStr );

	m_editEntity.SetFocus();

	return FALSE; // return TRUE unless you set the focus to a control
}

void CFindBrushDlg::OnBnClickedOk( void )
{
	CString entStr, brushStr;
	m_editEntity.GetWindowText( entStr );
	m_editBrush.GetWindowText( brushStr );

	SelectBrush( _ttoi( entStr ), _ttoi( brushStr ) );
	g_pParentWnd->OnViewCenter();

	CDialogEx::OnOK();
}

void CFindBrushDlg::OnBnClickedCancel()
{
	CDialogEx::OnCancel();
}