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

#undef nullptr

#include <io.h>
#include <fcntl.h>
#include <iostream>
#include <thread>
#include <conio.h>

#include "win_local.h"

char returnedText[512];
char consoleText[512];

void Sys_ConsoleInputThread();

/*
** Sys_BindCrtHandlesToStdHandles
** https://stackoverflow.com/questions/311955/redirecting-cout-to-a-console-in-windows
*/
void Sys_BindCrtHandlesToStdHandles( bool bindStdIn, bool bindStdOut, bool bindStdErr ) {
	/*
		Re-initialize the C runtime "FILE" handles with clean handles bound to "nul". We do this because it has been
		observed that the file number of our standard handle file objects can be assigned internally to a value of -2
		when not bound to a valid target, which represents some kind of unknown internal invalid state. In this state our
		call to "_dup2" fails, as it specifically tests to ensure that the target file number isn't equal to this value
		before allowing the operation to continue. We can resolve this issue by first "re-opening" the target files to
		use the "nul" device, which will place them into a valid state, after which we can redirect them to our target
		using the "_dup2" function.
	*/
	if ( bindStdIn ) {
		FILE *dummyFile;
		freopen_s( &dummyFile, "nul", "r", stdin );
	}

	if ( bindStdOut ) {
		FILE *dummyFile;
		freopen_s( &dummyFile, "nul", "w", stdout );
	}

	if ( bindStdErr ) {
		FILE *dummyFile;
		freopen_s( &dummyFile, "nul", "w", stderr );
	}

	// Redirect unbuffered stdin from the current standard input handle
	if ( bindStdIn ) {
		HANDLE stdHandle = GetStdHandle( STD_INPUT_HANDLE );
		if ( stdHandle != INVALID_HANDLE_VALUE ) {
			int fileDescriptor = _open_osfhandle( (intptr_t)stdHandle, _O_TEXT );
			if ( fileDescriptor != -1 ){
				FILE *file = _fdopen( fileDescriptor, "r" );
				if ( file != NULL ) {
					int dup2Result = _dup2( _fileno( file ), _fileno( stdin ) );
					if ( dup2Result == 0 ) {
						setvbuf( stdin, NULL, _IONBF, 0 );
					}
				}
			}
		}
	}

	// Redirect unbuffered stdout to the current standard output handle
	if ( bindStdOut ) {
		HANDLE stdHandle = GetStdHandle( STD_OUTPUT_HANDLE );
		if ( stdHandle != INVALID_HANDLE_VALUE ) {
			int fileDescriptor = _open_osfhandle( (intptr_t)stdHandle, _O_TEXT );
			if ( fileDescriptor != -1 ) {
				FILE *file = _fdopen( fileDescriptor, "w" );
				if ( file != NULL ) {
					int dup2Result = _dup2( _fileno( file ), _fileno( stdout ) );
					if ( dup2Result == 0 ) {
						setvbuf( stdout, NULL, _IONBF, 0 );
					}
				}
			}
		}
	}

	// Redirect unbuffered stderr to the current standard error handle
	if ( bindStdErr ) {
		HANDLE stdHandle = GetStdHandle(STD_ERROR_HANDLE);
		if ( stdHandle != INVALID_HANDLE_VALUE ) {
			int fileDescriptor = _open_osfhandle((intptr_t)stdHandle, _O_TEXT);
			if ( fileDescriptor != -1 ) {
				FILE *file = _fdopen( fileDescriptor, "w" );
				if ( file != NULL ) {
					int dup2Result = _dup2( _fileno( file ), _fileno( stderr ) );
					if ( dup2Result == 0 ) {
						setvbuf( stderr, NULL, _IONBF, 0 );
					}
				}
			}
		}
	}

	/*
		Clear the error state for each of the C++ standard stream objects. We need to do this, as attempts to access the
		standard streams before they refer to a valid target will cause the iostream objects to enter an error state. In
		versions of Visual Studio after 2005, this seems to always occur during startup regardless of whether anything
		has been read from or written to the targets or not.
	*/
	if ( bindStdIn ) {
		std::wcin.clear();
		std::cin.clear();
	}

	if ( bindStdOut ) {
		std::wcout.clear();
		std::cout.clear();
	}

	if ( bindStdErr ) {
		std::wcerr.clear();
		std::cerr.clear();
	}
}

/*
** Sys_CreateConsole
*/
void Sys_CreateConsole() {
	// We allocate our console first
	if( AllocConsole() ) {
		// Update the C/C++ runtime standard input, output, and error targets to use the console window
		Sys_BindCrtHandlesToStdHandles( true, true, true );
		SetConsoleTitle( "Console Output" );
		SetConsoleTextAttribute( GetStdHandle( STD_OUTPUT_HANDLE ), FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED );

		// Hide it by default
		HWND window = FindWindowA( "ConsoleWindowClass", NULL );
		ShowWindow( window, SW_HIDE );

		// Start the console input thread
		std::thread consoleInputThread( Sys_ConsoleInputThread );
		consoleInputThread.detach();
	}
}

/*
** Sys_DestroyConsole
*/
void Sys_DestroyConsole() {
	FreeConsole();
}

/*
** Sys_ShowConsole
*/
void Sys_ShowConsole() {

	HWND window = FindWindowA( "ConsoleWindowClass", NULL );
	if ( !window ) {
		return;
	}

	ShowWindow( window, SW_SHOWDEFAULT );
}

/*
** Sys_HideConsole
*/
void Sys_HideConsole() {

	HWND window = FindWindowA( "ConsoleWindowClass", NULL );
	if ( !window ) {
		return;
	}

	ShowWindow( window, SW_HIDE );
}

/*
** Sys_ConsoleInputThread
*/
void Sys_ConsoleInputThread() {
	static char buffer[512];

	DWORD bytesRead;
	HANDLE hConsole = GetStdHandle(STD_INPUT_HANDLE);

	while ( true ) {
		// Check if there is input available
		if ( _kbhit() ) {
			if ( ReadConsole( hConsole, buffer, sizeof( buffer ) - 1, &bytesRead, NULL ) ) {
				buffer[bytesRead - 2] = '\0'; // Null-terminate the string and remove CRLF

				Sys_MutexLock( win32.criticalSections[CRITICAL_SECTION_ONE], true );
				strcpy( returnedText, buffer );
				Sys_MutexUnlock( win32.criticalSections[CRITICAL_SECTION_ONE] );
			}
		}
		Sys_Sleep( 100 );
	}
}

/*
** Sys_ConsoleInput
*/
char *Sys_ConsoleInput() {
#ifndef _DEBUG
	// Dont waste resources if we didn't have the log opened
	if ( !win32.win_viewlog.GetBool() ) {
		return NULL;
	}
#endif

	Sys_MutexLock( win32.criticalSections[CRITICAL_SECTION_ONE], false );

	if ( returnedText[0] == 0 ) {
		Sys_MutexUnlock( win32.criticalSections[CRITICAL_SECTION_ONE] );
		return NULL;
	}

	strcpy( consoleText, returnedText );
	returnedText[0] = 0;

	Sys_MutexUnlock( win32.criticalSections[CRITICAL_SECTION_ONE] );

	return consoleText;
}


/*
** Conbuf_AppendText
*/
void Conbuf_AppendText( const char *pMsg )
{
#define CONSOLE_BUFFER_SIZE		16384

	char buffer[CONSOLE_BUFFER_SIZE*2];
	char *b = buffer;
	const char *msg;
	int bufLen;
	int i = 0;
	static unsigned long s_totalChars;

	//
	// if the message is REALLY long, use just the last portion of it
	//
	if ( strlen( pMsg ) > CONSOLE_BUFFER_SIZE - 1 )	{
		msg = pMsg + strlen( pMsg ) - CONSOLE_BUFFER_SIZE + 1;
	} else {
		msg = pMsg;
	}

	//
	// copy into an intermediate buffer
	//
	while ( msg[i] && ( ( b - buffer ) < sizeof( buffer ) - 1 ) ) {
		if ( msg[i] == '\n' && msg[i+1] == '\r' ) {
			b[0] = '\r';
			b[1] = '\n';
			b += 2;
			i++;
		} else if ( msg[i] == '\r' ) {
			b[0] = '\r';
			b[1] = '\n';
			b += 2;
		} else if ( msg[i] == '\n' ) {
			b[0] = '\r';
			b[1] = '\n';
			b += 2;
		} else if ( idStr::IsColor( &msg[i] ) ) {
			i++;
		} else {
			*b= msg[i];
			b++;
		}
		i++;
	}
	*b = 0;
	bufLen = b - buffer;

	s_totalChars += bufLen;

	//
	// replace selection instead of appending if we're overflowing
	//
	if ( s_totalChars > 0x7000 ) {
		s_totalChars = bufLen;
	}

	printf( buffer );
}