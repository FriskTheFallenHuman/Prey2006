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

// Set this to true to skip ALL assertions, including ones YOU CAUSE!
static volatile bool skipAllAssertions = false;

// Set this to true to skip ONLY this assertion
static volatile bool skipThisAssertion = false;

// Set this to true to manually break into the debugger
static volatile bool breakIntoDebugger = false;

#if defined( ID_PC_WIN )
struct AssertDlgInfo_s
{
	const char *file;
	int line;
	const char *expression;
};

static AssertDlgInfo_s assertInfo;
#endif

/*
================================================================================================
Contains the AssertMacro implementation.
================================================================================================
*/

idCVar com_assertOutOfDebugger( "com_assertOutOfDebugger", "0", CVAR_BOOL, "by default, do not assert while not running under the debugger" );

struct skippedAssertion_t {
					skippedAssertion_t() :
						file( NULL ),
						line( -1 ) {
					}
	const char *	file;
	int				line;
};
static idStaticList< skippedAssertion_t,20 > skippedAssertions;

#if defined( ID_PC_WIN )
/*
========================
AssertDialogProc

Shamelessly copied from DoomEdit

@inputs: HWND hDlg - handle to dialog box
@inputs: UINT uMsg - message
@inputs: WPARAM wParam - first message parameter
@inputs: LPARAM lParam - second message parameter
========================
*/
INT_PTR CALLBACK AssertDialogProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam ) {
	switch ( uMsg ) {
		case WM_INITDIALOG: {
			SetDlgItemText( hDlg, 4013 /*IDC_ASSERT_MSG_CTRL*/, assertInfo.expression );
			SetDlgItemText( hDlg, 4006 /*IDC_FILENAME*/ , assertInfo.file );
			SetDlgItemInt( hDlg, 4008 /*IDC_LINE_CONTROL*/, assertInfo.line, false );

			// Center the dialog.
			RECT rcDlg, rcDesktop;
			GetWindowRect( hDlg, &rcDlg );
			GetWindowRect( GetDesktopWindow(), &rcDesktop );
			SetWindowPos( hDlg, HWND_TOP, ( ( rcDesktop.right-rcDesktop.left ) - ( rcDlg.right-rcDlg.left ) ) / 2, ( ( rcDesktop.bottom-rcDesktop.top ) - ( rcDlg.bottom-rcDlg.top ) ) / 2, 0, 0, SWP_NOSIZE );
		}
		return true;

		case WM_COMMAND: {
			switch( LOWORD( wParam ) )
			{
				case 4009 /*IDC_SKIP_ASSERT*/: {
					skipThisAssertion = true;
					EndDialog( hDlg, 0 );
					return true;
				}

				case 4011 /*IDC_SKIP_ALL*/: {
					skipAllAssertions = false;
					EndDialog( hDlg, 0 );
					return true;
				}

				case 4010 /*IDC_BREAK*/: {
					breakIntoDebugger = true;
					EndDialog( hDlg, 0 );
					return true;
				}
			}

			case WM_KEYDOWN: {
				// Escape?
				if ( wParam == 2 ) {
					// Ignore this assert.
					EndDialog( hDlg, 0 );
					return true;
				}
			}
		}
		return true;
	}

	return FALSE;
}


static HWND hwndParentWnd;
static HINSTANCE hParentInstance;

/*
========================
ParentWindowEnumProc

base on the comments of: https://stackoverflow.com/questions/16872126/the-correct-way-of-getting-the-parent-window

@inputs: HWND hWnd - handle to parent window
@inputs: LPARAM lParam - application-defined value
========================
*/
static BOOL CALLBACK ParentWindowEnumProc( HWND hWnd, LPARAM lParam ) {
	if( IsWindowVisible( hWnd ) ) {
		DWORD procID;
		GetWindowThreadProcessId( hWnd, &procID );
		if ( procID == (DWORD)lParam ) {
			hwndParentWnd = hWnd;
			return FALSE; // don't iterate any more.
		}
	}
	return TRUE;
}

/*
========================
FindParentWindow

Enumerate top-level windows and take the first visible one with our processID.

base on the comments of: https://stackoverflow.com/questions/16872126/the-correct-way-of-getting-the-parent-window
========================
*/
HWND FindParentWindow() {
	hwndParentWnd = NULL;
	EnumWindows( ParentWindowEnumProc, GetCurrentProcessId() );
	return hwndParentWnd;
}

/*
========================
GetApplicationInstance

Get the HINSTANCE of the current application.
========================
*/
HINSTANCE GetApplicationInstance() {
	hParentInstance = GetModuleHandle( NULL );
	return hParentInstance;
}
#endif


/*
========================
AssertFailed
========================
*/
bool AssertFailed( const char * file, int line, const char * expression ) {
	if ( skipAllAssertions ) {
		return false;
	}

	skipThisAssertion = false;

	for ( int i = 0; i < skippedAssertions.Num(); i++ ) {
		if ( skippedAssertions[i].file == file && skippedAssertions[i].line == line ) {
			skipThisAssertion = true;
			// Set breakpoint here to re-enable
			if ( !skipThisAssertion ) {
				skippedAssertions.RemoveIndexFast( i );
			}
			return false;
		}
	}

	idLib::Warning( "ASSERTION FAILED! %s(%d): '%s'", file, line, expression );

#if defined( ID_PC_WIN )
	assertInfo.file = file;
	assertInfo.line = line;
	assertInfo.expression = expression;

	if ( !idLib::IsMainThread() ) {
		int bRet = MessageBox( NULL, expression, "Assertion Failed!", MB_SYSTEMMODAL | MB_CANCELTRYCONTINUE );
		if ( bRet == IDCANCEL ) {
			skipThisAssertion = true;
		} else if ( bRet == IDCONTINUE ) {
			breakIntoDebugger = true;
		}
	} else {
		HWND hParentWindow = FindParentWindow();
		HINSTANCE hParentInstance = GetApplicationInstance();
		DialogBox( hParentInstance, MAKEINTRESOURCE( 4004 /* IDD_ASSERT_DIALOG */ ), hParentWindow, AssertDialogProc);
	}
#endif

	if ( IsDebuggerPresent() && breakIntoDebugger || com_assertOutOfDebugger.GetBool() ) {
			__debugbreak();
	}

	if ( skipThisAssertion ) {
		skippedAssertion_t * skipped = skippedAssertions.Alloc();
		skipped->file = file;
		skipped->line = line;
	}

	return true;
}

