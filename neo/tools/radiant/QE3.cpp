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
#include <sys/stat.h>

QEGlobals_t g_qeglobals;

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void WINAPI QE_CheckOpenGLForErrors( void )
{
	CString strMsg;
	int		i = qglGetError();
	if( i != GL_NO_ERROR )
	{
		if( i == GL_OUT_OF_MEMORY )
		{
			//
			// strMsg.Format("OpenGL out of memory error %s\nDo you wish to save before
			// exiting?", gluErrorString((GLenum)i));
			//
			if( g_pParentWnd->MessageBox( strMsg, EDITOR_WINDOWTEXT " Error", MB_YESNO ) == IDYES )
			{
				Map_SaveFile( NULL, false );
			}

			exit( 1 );
		}
		else
		{
			// strMsg.Format("Warning: OpenGL Error %s\n ", gluErrorString((GLenum)i));
			common->Printf( strMsg.GetBuffer( 0 ) );
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Map_Snapshot()
{
	//
	// we need to do the following 1. make sure the snapshot directory exists (create
	// it if it doesn't) 2. find out what the lastest save is based on number 3. inc
	// that and save the map
	//
	idStr	   strOrgPath, strOrgFile;
	const auto mappath = idStr( currentmap ).BackSlashesToSlashes();
	mappath.ExtractFilePath( strOrgPath );
	mappath.ExtractFileBase( strOrgFile );

	strOrgPath.AppendPath( "snapshots" );

	idStr strNewPath = strOrgPath;
	strNewPath.AppendPath( strOrgFile );

	idStr test;
	for( int i = 0;; ++i )
	{
		sprintf( test, "%s__%i.map", strNewPath.c_str(), i );
		if( !Sys_IsFile( test.c_str() ) )
		{
			break;
		}
	}

	// strFile has the next available slot
	Map_SaveFile( test, false );
	Sys_SetTitle( currentmap );
}

/*
 =======================================================================================================================
	QE_CheckAutoSave If five minutes have passed since making a change and the map hasn't been saved, save it out.
 =======================================================================================================================
 */
void QE_CheckAutoSave( void )
{
	static bool inAutoSave = false;
	static bool autoToggle = false;
	if( inAutoSave )
	{
		Sys_Status( "Did not autosave due recursive entry into autosave routine\n" );
		return;
	}

	if( !mapModified )
	{
		return;
	}

	inAutoSave = true;

	if( g_PrefsDlg.m_bAutoSave )
	{
		Sys_Status( g_PrefsDlg.m_bSnapShots ? "Autosaving snapshot..." : "Autosaving...", 0 );

		if( g_PrefsDlg.m_bSnapShots && stricmp( currentmap, "unnamed.map" ) != 0 )
		{
			Map_Snapshot();
		}
		else
		{
			Map_SaveFile( ( autoToggle == 0 ) ? "autosave1" : "autosave2", false, true );
			autoToggle ^= 1;
		}
		Sys_Status( "Autosaving...Saved.", 0 );
		mapModified = 0; // DHM - _D3XP
	}
	else
	{
		common->Printf( "Autosave skipped...\n" );
		Sys_Status( "Autosave skipped...", 0 );
	}

	inAutoSave = false;
}

/*
 =======================================================================================================================
	ConnectEntities Sets target / name on the two entities selected from the first selected to the secon
 =======================================================================================================================
 */
void ConnectEntities( void )
{
	idEditorEntity* e1;
	const char*		target;
	idStr			strTarget;
	int				i, t;

	if( g_qeglobals.d_select_count < 2 )
	{
		MessageBoxA( g_pParentWnd->GetSafeHwnd(), "Must have at least two brushes selected.", "Can't Connect Entity", MB_OK | MB_ICONINFORMATION );
		return;
	}

	e1 = g_qeglobals.d_select_order[0]->owner;

	for( i = 0; i < g_qeglobals.d_select_count; i++ )
	{
		if( g_qeglobals.d_select_order[i]->owner == world_entity )
		{
			MessageBoxA( g_pParentWnd->GetSafeHwnd(), "Can't connect to the world.", "Can't Connect Entity", MB_OK | MB_ICONWARNING );
			return;
		}
	}

	for( i = 1; i < g_qeglobals.d_select_count; i++ )
	{
		if( e1 == g_qeglobals.d_select_order[i]->owner )
		{
			MessageBoxA( g_pParentWnd->GetSafeHwnd(), "Brushes are from same entity.", "Can't Connect Entity", MB_OK | MB_ICONINFORMATION );
			return;
		}
	}

	target = e1->ValueForKey( "target" );
	if( target && *target )
	{
		for( t = 1; t < 2048; t++ )
		{
			target = e1->ValueForKey( va( "target%i", t ) );
			if( target && *target )
			{
				continue;
			}
			else
			{
				break;
			}
		}
	}
	else
	{
		t = 0;
	}

	for( i = 1; i < g_qeglobals.d_select_count; i++ )
	{
		target = g_qeglobals.d_select_order[i]->owner->ValueForKey( "name" );
		if( target && *target )
		{
			strTarget = target;
		}
		else
		{
			UniqueTargetName( strTarget );
		}
		if( t == 0 )
		{
			e1->SetKeyValue( "target", strTarget );
		}
		else
		{
			e1->SetKeyValue( va( "target%i", t ), strTarget );
		}
		t++;
	}

	Sys_UpdateWindows( W_XY | W_CAMERA );

	Select_Deselect();
	Select_Brush( g_qeglobals.d_select_order[1] );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
bool QE_SingleBrush( bool bQuiet, bool entityOK )
{
	if( ( selected_brushes.next == &selected_brushes ) || ( selected_brushes.next->next != &selected_brushes ) )
	{
		if( !bQuiet )
		{
			MessageBoxA( g_pParentWnd->GetSafeHwnd(), "You must have a single brush selected.", "Brush Manipulation", MB_OK | MB_ICONERROR );
		}

		return false;
	}

	if( !entityOK && selected_brushes.next->owner->eclass->fixedsize )
	{
		if( !bQuiet )
		{
			MessageBoxA( g_pParentWnd->GetSafeHwnd(), "You cannot manipulate fixed size entities.", "Brush Manipulation", MB_OK | MB_ICONERROR );
		}

		return false;
	}

	return true;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void QE_Init( void )
{
	g_qeglobals.d_gridsize = 8;
	g_qeglobals.d_showgrid = true;

	Eclass_InitForSourceDirectory();
	g_Inspectors->FillClassList(); // list in entity window

	Map_New();

	/*
	 * other stuff
	 * FIXME: idMaterial Texture_Init (true); Cam_Init (); XY_Init ();
	 */
	Z_Init();
}

int	 g_numbrushes, g_numentities;

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void QE_CountBrushesAndUpdateStatusBar( void )
{
	static int	   s_lastbrushcount, s_lastentitycount;
	static bool	   s_didonce;

	// idEditorEntity *e;
	idEditorBrush *b, *next;

	g_numbrushes  = 0;
	g_numentities = 0;

	if( active_brushes.next != NULL )
	{
		for( b = active_brushes.next; b != NULL && b != &active_brushes; b = next )
		{
			next = b->next;
			if( b->brush_faces )
			{
				if( !b->owner->eclass->fixedsize )
				{
					g_numbrushes++;
				}
				else
				{
					g_numentities++;
				}
			}
		}
	}

	/*
	 * if ( entities.next != NULL ) { for ( e = entities.next ; e != &entities &&
	 * g_numentities != MAX_MAP_ENTITIES ; e = e->next) { g_numentities++; } }
	 */
	if( ( ( g_numbrushes != s_lastbrushcount ) || ( g_numentities != s_lastentitycount ) ) || ( !s_didonce ) )
	{
		Sys_UpdateStatusBar();

		s_lastbrushcount  = g_numbrushes;
		s_lastentitycount = g_numentities;
		s_didonce		  = true;
	}
}
