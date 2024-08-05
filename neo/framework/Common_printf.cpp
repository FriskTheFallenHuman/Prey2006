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

#include "precompiled.h"
#pragma hdrstop

#include "Common_local.h"

idCVar com_timestampPrints( "com_timestampPrints", "0", CVAR_SYSTEM, "print time with each console print, 1 = msec, 2 = sec", 0, 2, idCmdSystem::ArgCompletion_Integer<0,2> );
#ifndef ID_RETAIL
idCVar com_printFilter( "com_printFilter", "", CVAR_SYSTEM, "only print lines that contain this, add multiple filters with a ; delimeter");
#endif

#ifdef _WIN32
unsigned int	com_msgID = -1;
#endif

/*
==================
idCommonLocal::BeginRedirect
==================
*/
void idCommonLocal::BeginRedirect( char *buffer, int buffersize, void (*flush)( const char *) ) {
	if ( !buffer || !buffersize || !flush ) {
		return;
	}
	rd_buffer = buffer;
	rd_buffersize = buffersize;
	rd_flush = flush;

	*rd_buffer = 0;
}

/*
==================
idCommonLocal::EndRedirect
==================
*/
void idCommonLocal::EndRedirect( void ) {
	if ( rd_flush && rd_buffer[ 0 ] ) {
		rd_flush( rd_buffer );
	}

	rd_buffer = NULL;
	rd_buffersize = 0;
	rd_flush = NULL;
}

/*
==================
idCommonLocal::SetRefreshOnPrint
==================
*/
void idCommonLocal::SetRefreshOnPrint( bool set ) {
	com_refreshOnPrint = set;
}

#ifdef _WIN32
/*
==================
EnumWindowsProc
==================
*/
BOOL CALLBACK EnumWindowsProc( HWND hwnd, LPARAM lParam ) {
	char buff[1024];

	::GetWindowText( hwnd, buff, sizeof( buff ) );
	if ( idStr::Icmpn( buff, EDITOR_WINDOWTEXT, strlen( EDITOR_WINDOWTEXT ) ) == 0 ) {
		com_hwndMsg = hwnd;
		return FALSE;
	}
	return TRUE;
}

/*
==================
FindEditor
==================
*/
bool FindEditor( void ) {
	com_hwndMsg = NULL;
	EnumWindows( EnumWindowsProc, 0 );
	return !( com_hwndMsg == NULL );
}
#endif

/*
==================
idCommonLocal::VPrintf

A raw string should NEVER be passed as fmt, because of "%f" type crashes.
==================
*/
void idCommonLocal::VPrintf( const char *fmt, va_list args ) {
	char		msg[MAX_PRINT_MSG_SIZE];
	int			timeLength;

	// if the cvar system is not initialized
	if ( !cvarSystem->IsInitialized() ) {
		return;
	}

	// optionally put a timestamp at the beginning of each print,
	// so we can see how long different init sections are taking
	if ( com_timestampPrints.GetInteger() ) {
		int	t = Sys_Milliseconds();
		if ( com_timestampPrints.GetInteger() == 1 ) {
			t /= 1000;
		}
		sprintf( msg, "[%i]", t );
		timeLength = strlen( msg );
	} else {
		timeLength = 0;
	}

	// don't overflow
	if ( idStr::vsnPrintf( msg+timeLength, MAX_PRINT_MSG_SIZE-timeLength-1, fmt, args ) < 0 ) {
		msg[sizeof(msg)-2] = '\n'; msg[sizeof(msg)-1] = '\0'; // avoid output garbling
		Sys_Printf( "idCommon::VPrintf: truncated to %zd characters\n", strlen(msg)-1 );
	}

	if ( rd_buffer ) {
		if ( (int)( strlen( msg ) + strlen( rd_buffer ) ) > ( rd_buffersize - 1 ) ) {
			rd_flush( rd_buffer );
			*rd_buffer = 0;
		}
		strcat( rd_buffer, msg );
		return;
	}

#ifndef ID_RETAIL
	if ( com_printFilter.GetString() != NULL && com_printFilter.GetString()[ 0 ] != '\0' ) {
		idStr filterBuf = com_printFilter.GetString();
		idStr msgBuf = msg;
		filterBuf.ToLower();
		msgBuf.ToLower();
		char *sp = strtok( &filterBuf[ 0 ], ";" );
		bool p = false;
		for( ; sp != NULL ; ) {
			if ( strstr( msgBuf, sp ) != NULL ) {
				p = true;
				break;
			}
			sp = strtok( NULL, ";" );
		}
		if ( !p ) {
			return;
		}
	}
#endif

	// echo to console buffer
	console->Print( msg );

	// remove any color codes
	idStr::RemoveColors( msg );

	if ( com_enableDebuggerServer.GetBool( ) ) 	{
		// print to script debugger server
		if ( com_editors & EDITOR_DEBUGGER )
			DebuggerServerPrint( msg );
		else
			// only echo to dedicated console and early console when debugger is not running so no 
			// deadlocks occur if engine functions called from the debuggerthread trace stuff..
			Sys_Printf( "%s", msg );
	} else {
		Sys_Printf( "%s", msg );
	}
#if 0	// !@#
#if defined(_DEBUG) && defined(WIN32)
	if ( strlen( msg ) < 512 ) {
		TRACE( msg );
	}
#endif
#endif

	// don't trigger any updates if we are in the process of doing a fatal error
	if ( com_errorEntered != ERP_FATAL ) {
		// update the console if we are in a long-running command, like dmap
		if ( com_refreshOnPrint ) {
			session->UpdateScreen();
		}

		// let session redraw the animated loading screen if necessary
		session->PacifierUpdate();
	}

#ifdef _WIN32
	if ( com_outputMsg ) {
		if ( com_msgID == -1 ) {
			com_msgID = ::RegisterWindowMessage( DMAP_MSGID );
			if ( !FindEditor() ) {
				com_outputMsg = false;
			} else {
				Sys_ShowWindow( false );
			}
		}
		if ( com_hwndMsg ) {
			ATOM atom = ::GlobalAddAtom( msg );
			::PostMessage( com_hwndMsg, com_msgID, 0, static_cast<LPARAM>(atom) );
		}
	}

#endif
}

/*
==================
idCommonLocal::Printf

Both client and server can use this, and it will output to the appropriate place.

A raw string should NEVER be passed as fmt, because of "%f" type crashers.
==================
*/
void idCommonLocal::Printf( const char *fmt, ... ) {
	va_list argptr;
	va_start( argptr, fmt );
	VPrintf( fmt, argptr );
	va_end( argptr );
}

/*
==================
idCommonLocal::DPrintf

prints message that only shows up if the "developer" cvar is set
==================
*/
void idCommonLocal::DPrintf( const char *fmt, ... ) {
	va_list		argptr;
	char		msg[MAX_PRINT_MSG_SIZE];

	if ( !cvarSystem->IsInitialized() || !com_developer.GetBool() ) {
		return;			// don't confuse non-developers with techie stuff...
	}

	va_start( argptr, fmt );
	idStr::vsnPrintf( msg, sizeof(msg), fmt, argptr );
	va_end( argptr );
	msg[sizeof(msg)-1] = '\0';

	// never refresh the screen, which could cause reentrency problems
	bool temp = com_refreshOnPrint;
	com_refreshOnPrint = false;

	Printf( S_COLOR_RED"%s", msg );

	com_refreshOnPrint = temp;
}

/*
==================
idCommonLocal::DWarning

prints warning message in yellow that only shows up if the "developer" cvar is set
==================
*/
void idCommonLocal::DWarning( const char *fmt, ... ) {
	va_list		argptr;
	char		msg[MAX_PRINT_MSG_SIZE];

	if ( !com_developer.GetBool() ) {
		return;			// don't confuse non-developers with techie stuff...
	}

	va_start( argptr, fmt );
	idStr::vsnPrintf( msg, sizeof(msg), fmt, argptr );
	va_end( argptr );
	msg[sizeof(msg)-1] = '\0';

	Printf( S_COLOR_YELLOW"WARNING: %s\n", msg );
}

/*
==================
idCommonLocal::Warning

prints WARNING %s and adds the warning message to a queue to be printed later on
==================
*/
void idCommonLocal::Warning( const char *fmt, ... ) {
	va_list		argptr;
	char		msg[MAX_PRINT_MSG_SIZE];

	va_start( argptr, fmt );
	idStr::vsnPrintf( msg, sizeof(msg), fmt, argptr );
	va_end( argptr );
	msg[sizeof(msg)-1] = 0;

	Printf( S_COLOR_YELLOW "WARNING: " S_COLOR_RED "%s\n", msg );

	if ( warningList.Num() < MAX_WARNING_LIST ) {
		warningList.AddUnique( msg );
	}
}

/*
==================
idCommonLocal::PrintWarnings
==================
*/
void idCommonLocal::PrintWarnings( void ) {
	int i;

	if ( !warningList.Num() ) {
		return;
	}

	warningList.Sort();

	Printf( "----- Warnings -----\n" );
	Printf( "during %s...\n", warningCaption.c_str() );

	for ( i = 0; i < warningList.Num(); i++ ) {
		Printf( S_COLOR_YELLOW "WARNING: " S_COLOR_RED "%s\n", warningList[i].c_str() );
	}
	if ( warningList.Num() ) {
		if ( warningList.Num() >= MAX_WARNING_LIST ) {
			Printf("----- more than %d warnings -----\n", MAX_WARNING_LIST );
		} else {
			if ( warningList.Num() > 1 ) {
				Printf("----- %d warnings -----\n", warningList.Num() );
			} else {
				Printf("----- %d warning -----\n", warningList.Num() );			
			}
		}
	}
}

/*
==================
idCommonLocal::ClearWarnings
==================
*/
void idCommonLocal::ClearWarnings( const char *reason ) {
	warningCaption = reason;
	warningList.Clear();
}

/*
==================
idCommonLocal::DumpWarnings
==================
*/
void idCommonLocal::DumpWarnings( void ) {
	int			i;
	idFile		*warningFile;

	if ( !warningList.Num() ) {
		return;
	}

	warningFile = fileSystem->OpenFileWrite( "warnings.txt", "fs_savepath" );
	if ( warningFile ) {

		warningFile->Printf( "----- Warnings -----\n\n" );
		warningFile->Printf( "during %s...\n", warningCaption.c_str() );
		warningList.Sort();
		for ( i = 0; i < warningList.Num(); i++ ) {
			warningList[i].RemoveColors();
			warningFile->Printf( "WARNING: %s\n", warningList[i].c_str() );
		}
		if ( warningList.Num() >= MAX_WARNING_LIST ) {
			warningFile->Printf("----- more than %d warnings -----\n", MAX_WARNING_LIST );
		} else {
			if ( warningList.Num() > 1 ) {
				warningFile->Printf("----- %d warnings -----\n", warningList.Num() );
			} else {
				warningFile->Printf("----- %d warning -----\n", warningList.Num() );
			}
		}

		warningFile->Printf( "\n\n----- Errors -----\n\n" );
		errorList.Sort();
		for ( i = 0; i < errorList.Num(); i++ ) {
			errorList[i].RemoveColors();
			warningFile->Printf( "ERROR: %s", errorList[i].c_str() );
		}

		warningFile->ForceFlush();

		fileSystem->CloseFile( warningFile );

#if defined(_WIN32) && !defined(_DEBUG)
		idStr	osPath;
		osPath = fileSystem->RelativePathToOSPath( "warnings.txt", "fs_savepath" );
		WinExec( va( "Notepad.exe %s", osPath.c_str() ), SW_SHOW );
#endif
	}
}

/*
==================
idCommonLocal::Error
==================
*/
void idCommonLocal::Error( const char *fmt, ... ) {
	va_list		argptr;
	static int	lastErrorTime;
	static int	errorCount;
	int			currentTime;

	int code = ERP_DROP;

	// always turn this off after an error
	com_refreshOnPrint = false;

	// when we are running automated scripts, make sure we
	// know if anything failed
	if ( cvarSystem->GetCVarInteger( "fs_copyfiles" ) ) {
		code = ERP_FATAL;
	}

	// if we don't have GL running, make it a fatal error
	if ( !renderSystem->IsOpenGLRunning() ) {
		code = ERP_FATAL;
	}

	// if we got a recursive error, make it fatal
	if ( com_errorEntered ) {
		// if we are recursively erroring while exiting
		// from a fatal error, just kill the entire
		// process immediately, which will prevent a
		// full screen rendering window covering the
		// error dialog
		if ( com_errorEntered == ERP_FATAL ) {
			Sys_Quit();
		}
		code = ERP_FATAL;
	}

	// if we are getting a solid stream of ERP_DROP, do an ERP_FATAL
	currentTime = Sys_Milliseconds();
	if ( currentTime - lastErrorTime < 100 ) {
		if ( ++errorCount > 3 ) {
			code = ERP_FATAL;
		}
	} else {
		errorCount = 0;
	}
	lastErrorTime = currentTime;

	com_errorEntered = code;

	va_start (argptr,fmt);
	idStr::vsnPrintf( errorMessage, sizeof(errorMessage), fmt, argptr );
	va_end (argptr);
	errorMessage[sizeof(errorMessage)-1] = '\0';

	// copy the error message to the clip board
	Sys_SetClipboardData( errorMessage );

	// add the message to the error list
	errorList.AddUnique( errorMessage );

	// Dont shut down the session for gui editor or debugger
	if ( !( com_editors & ( EDITOR_GUI | EDITOR_DEBUGGER ) ) ) {
		session->Stop();
	}

	if ( code == ERP_DISCONNECT ) {
		com_errorEntered = 0;
		throw idException( errorMessage );
	// The gui editor doesnt want thing to com_error so it handles exceptions instead
	} else if( com_editors & ( EDITOR_GUI | EDITOR_DEBUGGER ) ) {
		com_errorEntered = 0;
		throw idException( errorMessage );
	} else if ( code == ERP_DROP ) {
		Printf( "********************\nERROR: %s\n********************\n", errorMessage );
		com_errorEntered = 0;
		throw idException( errorMessage );
	} else {
		Printf( "********************\nERROR: %s\n********************\n", errorMessage );
	}

	if ( cvarSystem->GetCVarBool( "r_fullscreen" ) ) {
		cmdSystem->BufferCommandText( CMD_EXEC_NOW, "vid_restart partial windowed\n" );
	}

	Shutdown();

	Sys_Error( "%s", errorMessage );
}

/*
==================
idCommonLocal::FatalError

Dump out of the game to a system dialog
==================
*/
void idCommonLocal::FatalError( const char *fmt, ... ) {
	va_list		argptr;

	// if we got a recursive error, make it fatal
	if ( com_errorEntered ) {
		// if we are recursively erroring while exiting
		// from a fatal error, just kill the entire
		// process immediately, which will prevent a
		// full screen rendering window covering the
		// error dialog

		Sys_Printf( "FATAL: recursed fatal error:\n%s\n", errorMessage );

		va_start( argptr, fmt );
		idStr::vsnPrintf( errorMessage, sizeof(errorMessage), fmt, argptr );
		va_end( argptr );
		errorMessage[sizeof(errorMessage)-1] = '\0';

		Sys_Printf( "%s\n", errorMessage );

		// write the console to a log file?
		Sys_Quit();
	}
	com_errorEntered = ERP_FATAL;

	va_start( argptr, fmt );
	idStr::vsnPrintf( errorMessage, sizeof(errorMessage), fmt, argptr );
	va_end( argptr );
	errorMessage[sizeof(errorMessage)-1] = '\0';

	if ( cvarSystem->GetCVarBool( "r_fullscreen" ) ) {
		cmdSystem->BufferCommandText( CMD_EXEC_NOW, "vid_restart partial windowed\n" );
	}

	Sys_Printf( "shutting down: %s\n", errorMessage );

	Shutdown();

	Sys_Error( "%s", errorMessage );
}