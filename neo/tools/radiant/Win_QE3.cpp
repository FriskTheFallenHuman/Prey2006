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
#include "mru.h"

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Sys_MarkMapModified( void )
{
	idStr title;

	if( mapModified != 1 )
	{
		mapModified = 1; // mark the map as changed
		title		= currentmap;
		title += " *";
		title.BackSlashesToSlashes();
		Sys_SetTitle( title );
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Sys_SetTitle( const char* text )
{
	g_pParentWnd->SetWindowText( va( "%s: %s", EDITOR_WINDOWTEXT, text ) );
}

/*
 =======================================================================================================================
 Wait Functions
 =======================================================================================================================
 */
HCURSOR waitcursor;

void	Sys_BeginWait( void )
{
	waitcursor = SetCursor( LoadCursor( NULL, IDC_WAIT ) );
}

bool Sys_Waiting()
{
	return ( waitcursor != NULL );
}

void Sys_EndWait( void )
{
	if( waitcursor )
	{
		SetCursor( waitcursor );
		waitcursor = NULL;
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Sys_GetCursorPos( int* x, int* y )
{
	POINT lpPoint;

	GetCursorPos( &lpPoint );
	*x = lpPoint.x;
	*y = lpPoint.y;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Sys_SetCursorPos( int x, int y )
{
	SetCursorPos( x, y );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
double Sys_DoubleTime( void )
{
	return clock() / 1000.0;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
int WINAPI QEW_SetupPixelFormat( HDC hDC, bool zbuffer )
{
	int pixelFormat = Win_ChoosePixelFormat( hDC );
	if( pixelFormat > 0 )
	{
		if( SetPixelFormat( hDC, pixelFormat, &win32.pfd ) == NULL )
		{
			idLib::Error( "SetPixelFormat failed." );
		}
	}
	else
	{
		idLib::Error( "ChoosePixelFormat failed." );
	}

	return pixelFormat;
}

/*
 =======================================================================================================================
	FILE DIALOGS
 =======================================================================================================================
 */
bool ConfirmModified( void )
{
	if( !mapModified )
	{
		return true;
	}

	if( g_pParentWnd->MessageBox( "This will lose changes to the map.", "Unsaved Changes", MB_OKCANCEL | MB_ICONWARNING ) == IDCANCEL )
	{
		return false;
	}

	return true;
}

static OPENFILENAME ofn;													  /* common dialog box structure */
static char			szFile[MAX_PATH];										  /* filename string */
static char			szFileTitle[260];										  /* file title string */
static char			szFilter[260] = "Map file (*.map, *.reg)\0*.map;*.reg\0"; /* filter string */

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void				OpenDialog( void )
{
	/* Place the terminating null character in the szFile. */
	szFile[0] = '\0';

	/* Set the members of the OPENFILENAME structure. */
	ZeroMemory( &ofn, sizeof( OPENFILENAME ) );
	ofn.lStructSize		= sizeof( OPENFILENAME );
	ofn.hwndOwner		= g_pParentWnd->GetSafeHwnd();
	ofn.lpstrFilter		= szFilter;
	ofn.nFilterIndex	= 1;
	ofn.lpstrFile		= szFile;
	ofn.nMaxFile		= sizeof( szFile );
	ofn.lpstrFileTitle	= szFileTitle;
	ofn.nMaxFileTitle	= sizeof( szFileTitle );
	ofn.lpstrInitialDir = NULL;
	ofn.Flags			= OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	/* Display the Open dialog box. */
	if( !GetOpenFileName( &ofn ) )
	{
		return; // canceled
	}

	// Add the file in MRU. FIXME
	AddNewItem( g_qeglobals.d_lpMruMenu, ofn.lpstrFile );

	// Refresh the File menu. FIXME
	PlaceMenuMRUItem( g_qeglobals.d_lpMruMenu, GetSubMenu( GetMenu( g_pParentWnd->GetSafeHwnd() ), 0 ), ID_FILE_EXIT );

	/* Open the file. */
	Map_LoadFile( ofn.lpstrFile );

	g_PrefsDlg.SavePrefs();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void SaveAsDialog( bool bRegion )
{
	/* Place the terminating null character in the szFile. */
	szFile[0] = '\0';

	/* Set the members of the OPENFILENAME structure. */
	ZeroMemory( &ofn, sizeof( OPENFILENAME ) );
	ofn.lStructSize		= sizeof( OPENFILENAME );
	ofn.hwndOwner		= g_pParentWnd->GetSafeHwnd();
	ofn.lpstrFilter		= szFilter;
	ofn.nFilterIndex	= 1;
	ofn.lpstrFile		= szFile;
	ofn.nMaxFile		= sizeof( szFile );
	ofn.lpstrFileTitle	= szFileTitle;
	ofn.nMaxFileTitle	= sizeof( szFileTitle );
	ofn.lpstrInitialDir = "";
	ofn.Flags			= OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_OVERWRITEPROMPT;

	/* Display the Open dialog box. */
	if( !GetSaveFileName( &ofn ) )
	{
		return; // canceled
	}

	if( bRegion )
	{
		DefaultExtension( ofn.lpstrFile, ".reg" );
	}
	else
	{
		DefaultExtension( ofn.lpstrFile, ".map" );
	}

	if( !bRegion )
	{
		strcpy( currentmap, ofn.lpstrFile );
		AddNewItem( g_qeglobals.d_lpMruMenu, ofn.lpstrFile );
		PlaceMenuMRUItem( g_qeglobals.d_lpMruMenu, GetSubMenu( GetMenu( g_pParentWnd->GetSafeHwnd() ), 0 ), ID_FILE_EXIT );
	}

	Map_SaveFile( ofn.lpstrFile, bRegion ); // ignore region
}
