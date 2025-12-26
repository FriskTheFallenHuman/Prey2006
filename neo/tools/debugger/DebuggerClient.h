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
#ifndef DEBUGGERCLIENT_H_
#define DEBUGGERCLIENT_H_

#ifndef DEBUGGERBREAKPOINT_H_
#include "DebuggerBreakpoint.h"
#endif

class rvDebuggerCallstack {
public:

	idStr	mFilename;
	int		mLineNumber;
	idStr	mFunction;
};

class rvDebuggerThread {
public:

	idStr	mName;
	int		mID;
	bool	mCurrent;
	bool	mDying;
	bool	mWaiting;
	bool	mDoneProcessing;
};


typedef idList<rvDebuggerCallstack *>	rvDebuggerCallstackList;
typedef idList<rvDebuggerThread *>		rvDebuggerThreadList;
typedef idList<rvDebuggerBreakpoint *>	rvDebuggerBreakpointList;

class rvDebuggerClient {
public:

	rvDebuggerClient( );
	~rvDebuggerClient( );

	bool						Initialize();
	void						Shutdown();
	bool						ProcessMessages();
	bool						WaitFor( EDebuggerMessage msg, int time );

	bool						IsConnected();
	bool						IsStopped();

	int							GetActiveBreakpointID();
	const char			*		GetBreakFilename();
	int							GetBreakLineNumber();
	idProgram			*		GetBreakProgram();
	rvDebuggerCallstackList	& GetCallstack();
	rvDebuggerThreadList	&	GetThreads();
	const char			*		GetVariableValue( const char * name, int stackDepth );
	idStrList			&		GetServerScripts();

	void						InspectVariable( const char * name, int callstackDepth );
	void						InspectScripts();
	void						Break();
	void						Resume();
	void						StepInto();
	void						StepOver();

	void						SendCommand( const char * cmdStr );

	// Breakpoints
	int							AddBreakpoint( const char * filename, int lineNumber, bool onceOnly = false );
	bool						RemoveBreakpoint( int bpID );
	void						ClearBreakpoints();
	int							GetBreakpointCount();
	rvDebuggerBreakpoint	*	GetBreakpoint( int index );
	rvDebuggerBreakpoint	*	FindBreakpoint( const char * filename, int linenumber );

protected:

	void						SendMessage( EDebuggerMessage dbmsg );
	void						SendBreakpoints();
	void						SendAddBreakpoint( rvDebuggerBreakpoint& bp );
	void						SendRemoveBreakpoint( rvDebuggerBreakpoint& bp );
	void						SendPacket( void * data, int datasize );

	bool						mConnected;
	netadr_t					mServerAdr;
	idPort						mPort;

	bool						mBreak;
	int							mBreakID;
	int							mBreakLineNumber;
	idStr						mBreakFilename;

	idDict						mVariables;

	rvDebuggerCallstackList		mCallstack;
	rvDebuggerThreadList		mThreads;
	rvDebuggerBreakpointList	mBreakpoints;

	EDebuggerMessage			mWaitFor;

	idStrList					mServerScripts;

private:

	void		ClearCallstack();
	void		ClearThreads();

	void		UpdateWatches();

	// Network message handlers
	void		HandleBreak( idBitMsg* msg );
	void		HandleInspectScripts( idBitMsg* msg );
	void		HandleInspectCallstack( idBitMsg* msg );
	void		HandleInspectThreads( idBitMsg* msg );
	void		HandleInspectVariable( idBitMsg* msg );
	void		HandleGameDLLHandle( idBitMsg* msg );
	void		HandleRemoveBreakpoint( idBitMsg* msg );
};

/*
================
rvDebuggerClient::IsConnected
================
*/
ID_INLINE bool rvDebuggerClient::IsConnected() {
	return mConnected;
}

/*
================
rvDebuggerClient::IsStopped
================
*/
ID_INLINE bool rvDebuggerClient::IsStopped() {
	return mBreak;
}

/*
================
rvDebuggerClient::GetActiveBreakpointID
================
*/
ID_INLINE int rvDebuggerClient::GetActiveBreakpointID() {
	return mBreakID;
}

/*
================
rvDebuggerClient::GetBreakFilename
================
*/
ID_INLINE const char * rvDebuggerClient::GetBreakFilename() {
	return mBreakFilename;
}

/*
================
rvDebuggerClient::GetBreakLineNumber
================
*/
ID_INLINE int rvDebuggerClient::GetBreakLineNumber() {
	return mBreakLineNumber;
}

/*
================
rvDebuggerClient::GetCallstack
================
*/
ID_INLINE rvDebuggerCallstackList & rvDebuggerClient::GetCallstack() {
	return mCallstack;
}

/*
================
rvDebuggerClient::GetThreads
================
*/
ID_INLINE rvDebuggerThreadList & rvDebuggerClient::GetThreads() {
	return mThreads;
}

/*
================
rvDebuggerClient::GetVariableValue
================
*/
ID_INLINE const char * rvDebuggerClient::GetVariableValue( const char * var, int stackDepth ) {
	return mVariables.GetString( va( "%d:%s", stackDepth, var ), "" );
}

/*
================
rvDebuggerClient::GetBreakpointCount
================
*/
ID_INLINE int rvDebuggerClient::GetBreakpointCount() {
	return mBreakpoints.Num( );
}

/*
================
rvDebuggerClient::GetBreakpoint
================
*/
ID_INLINE rvDebuggerBreakpoint * rvDebuggerClient::GetBreakpoint( int index ) {
	return mBreakpoints[index];
}

/*
================
rvDebuggerClient::Break
================
*/
ID_INLINE void rvDebuggerClient::Break() {
	SendMessage( DBMSG_BREAK );
}

/*
================
rvDebuggerClient::Resume
================
*/
ID_INLINE void rvDebuggerClient::Resume() {
	mBreak = false;
	SendMessage( DBMSG_RESUME );
}

/*
================
rvDebuggerClient::StepOver
================
*/
ID_INLINE void rvDebuggerClient::StepOver() {
	mBreak = false;
	SendMessage( DBMSG_STEPOVER );
}

/*
================
rvDebuggerClient::StepInto
================
*/
ID_INLINE void rvDebuggerClient::StepInto() {
	mBreak = false;
	SendMessage( DBMSG_STEPINTO );
}

/*
================
rvDebuggerClient::SendPacket
================
*/
ID_INLINE void rvDebuggerClient::SendPacket( void * data, int size ) {
	mPort.SendPacket( mServerAdr, data, size );
}


/*
================
rvDebuggerClient::GetServerScripts
================
*/
ID_INLINE idStrList & rvDebuggerClient::GetServerScripts() {
	return mServerScripts;
}
#endif // DEBUGGERCLIENT_H_
