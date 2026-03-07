/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 BFG Edition GPL Source Code ("Doom 3 BFG Edition Source Code").

Doom 3 BFG Edition Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 BFG Edition Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 BFG Edition Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 BFG Edition Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/
#include "precompiled.h"
#pragma hdrstop

#include <string>
#include <map>

#if defined( ID_PC_WIN )
#include <CommCtrl.h>
#include <psapi.h>
#undef StrCmpI
#undef StrCmpNI
#undef StrCmpN
#include <shlwapi.h>
#endif

#if defined( ID_PC_WIN )
extern HWND FindParentWindow();
extern HINSTANCE GetApplicationInstance();

/*
========================
CrashHandlerDialogProc

Shamelessly copied from DoomEdit

@inputs: HWND hDlg - handle to dialog box
@inputs: UINT uMsg - message
@inputs: WPARAM wParam - first message parameter
@inputs: LPARAM lParam - second message parameter
========================
*/
INT_PTR CALLBACK CrashHandlerDialogProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam ) {
	switch ( uMsg ) {
		case WM_INITDIALOG: {
			// Initialize tab control
			HWND hTab = GetDlgItem( hDlg, 4002 /*ID_CRASH_TABS*/ );
			TCITEM tie;
			tie.mask = TCIF_TEXT;
			tie.pszText = "Call Stack";
			TabCtrl_InsertItem( hTab, 0, &tie );

			// Add data to ListView controls
			HWND hListView = GetDlgItem( hDlg, 4005 ); // ID_STACK_VIEW

			// Add extended styles to ListView
			ListView_SetExtendedListViewStyle( hListView, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_AUTOSIZECOLUMNS );

			// Setup ListView columns
			LVCOLUMN lvc;
			lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
			lvc.cx = 100;
			lvc.pszText = "Function";
			ListView_InsertColumn( hListView, 0, &lvc );

			lvc.cx = 50;
			lvc.pszText = "Line";
			ListView_InsertColumn( hListView, 1, &lvc );

			lvc.cx = 100;
			lvc.pszText = "Source File";
			ListView_InsertColumn( hListView, 2, &lvc );

			lvc.cx = 150;
			lvc.pszText = "Module";
			ListView_InsertColumn( hListView, 3, &lvc );

			// Capture stack trace and decode it
			uint8 data[4096];
			int len = sizeof( data );
			uint32 hash = idDebugSystem::GetStack( data, len );

			idList<debugStackFrame_t, TAG_CRAP> frames;
			idDebugSystem::DecodeStack( data, len, frames );

			// Clean the stack trace
			idDebugSystem::CleanStack( frames );
			frames.RemoveIndex( 0 );    //drop this function

			// Stringify the stack trace
			char str[4096];
			idDebugSystem::StringifyStack( hash, frames.Ptr(), frames.Num(), str, sizeof( str ) );

			// Enumerate modules and store their base names
			HMODULE hMods[1024];
			HANDLE hProcess = GetCurrentProcess();
			DWORD cbNeeded;
			std::map<void*, std::string> moduleMap;
			if ( EnumProcessModules( hProcess, hMods, sizeof( hMods ), &cbNeeded ) ) {
				for ( unsigned int i = 0; i < ( cbNeeded / sizeof( HMODULE ) ); i++ ) {
					char modName[MAX_PATH];
					if ( GetModuleFileNameExA( hProcess, hMods[i], modName, sizeof( modName ) / sizeof( char ) ) ) {
						void* baseAddress = reinterpret_cast<void*>( hMods[i] );
						moduleMap[baseAddress] = PathFindFileName( modName );
					}
				}
			}

			// Add items to ListView for Call Stack tab
			for ( int i = 0; i < frames.Num(); i++ ) {
				const debugStackFrame_t &fr = frames[i];

				char buffer[256];

				LVITEM lvi;
				lvi.mask = LVIF_TEXT;
				lvi.iItem = i;

				lvi.iSubItem = 0;
				if ( idStr::Cmp( fr.functionName, "[UNNAMED]" ) == 0 ) {
					strcpy( buffer, "**UNKNOWN**" );
				} else {
					strcpy( buffer, fr.functionName );
				}
				lvi.pszText = buffer;
				ListView_InsertItem( hListView, &lvi );

				lvi.iSubItem = 1;
				if ( fr.lineNumber == -1 ) {
					sprintf( buffer, "0" );
				} else {
					sprintf( buffer, "%d", fr.lineNumber );
				}
				lvi.pszText = buffer;
				ListView_SetItem( hListView, &lvi );

				lvi.iSubItem = 2;
				if ( idStr::Cmp( fr.fileName, "[UNNAMED]" ) == 0 || strlen( fr.fileName ) == 0 ) {
					sprintf( buffer, "0x%p", fr.pointer );
				} else {
					strcpy( buffer, fr.fileName );
				}
				lvi.pszText = buffer;
				ListView_SetItem( hListView, &lvi );

				// Get the module name for the pointer
				auto it = moduleMap.lower_bound( fr.pointer );
				if ( it != moduleMap.begin() && ( it == moduleMap.end() || it->first != fr.pointer ) ) {
					--it;
				}
				lvi.iSubItem = 3;
				lvi.pszText = ( it != moduleMap.end() ) ? const_cast<char*>( it->second.c_str(  ) ) : "[UNKNOWN]";
				ListView_SetItem( hListView, &lvi );
			}

			ShowWindow( hListView, SW_SHOW );

			// Center the dialog.
			RECT rcDlg, rcDesktop;
			GetWindowRect( hDlg, &rcDlg );
			GetWindowRect( GetDesktopWindow(), &rcDesktop );
			SetWindowPos( hDlg, HWND_TOP, ( ( rcDesktop.right-rcDesktop.left ) - ( rcDlg.right-rcDlg.left ) ) / 2, ( ( rcDesktop.bottom-rcDesktop.top ) - ( rcDlg.bottom-rcDlg.top ) ) / 2, 0, 0, SWP_NOSIZE );
		}
		return TRUE;

		case WM_COMMAND: {
			switch( LOWORD( wParam ) ) {
				case IDCLOSE: {
					EndDialog( hDlg, 0 );
					return TRUE;
				}
			}
		}
		return TRUE;

		case WM_MOUSEWHEEL: {
			HWND hFocused = GetFocus();
			if ( hFocused ) {
				SendMessage( hFocused, WM_MOUSEWHEEL, wParam, lParam );
			}
			return TRUE;
		}

		case WM_KEYDOWN: {
			// Escape?
			if ( wParam == 2 ) {
				EndDialog( hDlg, 0 );
				return TRUE;
			}
		}
		return TRUE;
	}

	return FALSE;
}
#endif

/*
================================================
idFatalException
================================================
*/
idFatalException::idFatalException( const char *text, bool doStackTrace, bool emergencyExit ) {
	strncpy( idException::error, text, MAX_ERROR_LEN );

	// Print our stack trace
	if ( doStackTrace ) {
		idLib::PrintCallStack();
	}

#if defined( ID_PC_WIN )
	if ( !idLib::IsMainThread() ) {
		int bRet = MessageBox( NULL, text, "Doom 3 BFG Unhandled Exception", MB_SYSTEMMODAL | MB_CANCELTRYCONTINUE );
		if ( bRet == IDCANCEL ) {
		} else if ( bRet == IDCONTINUE ) {
		}
	} else {
		HWND hParentWindow = FindParentWindow();
		HINSTANCE hParentInstance = GetApplicationInstance();
		DialogBox( hParentInstance, MAKEINTRESOURCE( 4001 /*IDD_CRASH_DIALOG*/ ), hParentWindow, CrashHandlerDialogProc );
	}
#endif

	// During emergency exiting, do not botter printing or do the error just quit
	if ( emergencyExit ) {
		Sys_Printf( "%s\n", text );

		// write the console to a log file?
		Sys_Quit();
	} else {
		Sys_Printf( "shutting down: %s\n", text );
		Sys_Error( "%s", text );
	}
}