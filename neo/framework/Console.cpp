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

#include "ConsoleHistory.h"

#define	MAX_PRINT_MSG		4096

#define	CON_TEXTSIZE			0x30000
#define	NUM_CON_TIMES			4
#define CONSOLE_FIRSTREPEAT		200
#define CONSOLE_REPEAT			100

#define	COMMAND_HISTORY			64

struct overlayText_t {
	idStr			text;
	justify_t		justify;
	textSize_t		size;
	int				time;
	bool			showbackground = false;
};

// the console will query the cvar and command systems for
// command completion information

class idConsoleLocal : public idConsole {
public:
	virtual	void		Init( void );
	virtual void		Shutdown( void );
	virtual	void		LoadGraphics( void );
	virtual	bool		ProcessEvent( const sysEvent_t *event, bool forceAccept );
	virtual	bool		Active( void );
	virtual	void		ClearNotifyLines( void );
	virtual	void		Close( void );
	virtual	void		Print( const char *text );
	virtual	void		Draw( bool forceFullScreen );

	virtual void		PrintOverlay( idOverlayHandle &handle, justify_t justify, const char *text, bool showbackground, textSize_t size, ... );

	void				Dump( const char *toFile );
	void				Clear();

	//============================

	const idMaterial *	charSetShader;
private:
	void				KeyDownEvent( int key );

	void				Linefeed();

	void				PageUp();
	void				PageDown();
	void				Top();
	void				Bottom();

	void				DrawInput();
	void				DrawNotify();
	void				DrawSolidConsole( float frac );

	void				Scroll();
	void				SetDisplayFraction( float frac );
	void				UpdateDisplayFraction( void );

	void				DrawTextSmallLeftAlign( float x, float &y, const char *text, ... );
	void				DrawTextSmallRightAlign( float x, float &y, const char *text, ... );
	void				DrawTextBigLeftAlign( float x, float &y, const char *text, ... );
	void				DrawTextBigRightAlign( float x, float &y, const char *text, ... );

	float				DrawFPS( float y );
	float				DrawMemoryUsage( float y );
	float				DrawAsyncStats( float y );
	float				DrawSoundDecoders( float y );

	void				DrawOverlayText( float & leftY, float & rightY, float & centerY );

	void				DrawFloatConsole( void );
	void				ZoomFloatConsole( int inOut );

	int					consoleY;
	int					consoleX;
	int					consoleW;
	int					consoleH;
	bool				drag;

	//============================

	// allow these constants to be adjusted for HMD
	int					LOCALSAFE_LEFT;
	int					LOCALSAFE_RIGHT;
	int					LOCALSAFE_TOP;
	int					LOCALSAFE_BOTTOM;
	int					LOCALSAFE_WIDTH;
	int					LOCALSAFE_HEIGHT;
	int					LINE_WIDTH;
	int					TOTAL_LINES;

	bool				keyCatching;

	short				text[CON_TEXTSIZE];
	int					current;		// line where next message will be printed
	int					x;				// offset in current line for next print
	int					display;		// bottom of console displays this line
	int					lastKeyEvent;	// time of last key event for scroll delay
	int					nextKeyEvent;	// keyboard repeat rate

	float				displayFrac;	// approaches finalFrac at con_speed
	float				finalFrac;		// 0.0 to 1.0 lines of console to display
	int					fracTime;		// time of last displayFrac update

	int					vislines;		// in scanlines

	int					times[NUM_CON_TIMES];	// cls.realtime time the line was generated
									// for transparent notify lines
	idVec4				color;

	idEditField			historyEditLines[COMMAND_HISTORY];

	int					nextHistoryLine;// the last line in the history buffer, not masked
	int					historyLine;	// the line being displayed from history buffer
									// will be <= nextHistoryLine

	idEditField			consoleField;

	idList< overlayText_t >	overlayText;

	static idCVar		con_speed;
	static idCVar		con_notifyTime;
	static idCVar		con_noPrint;
	static idCVar		con_size;
	static idCVar		con_floating;
	static idCVar		con_alwaysShow;
	static idCVar		con_floatingGeometry;
	static idCVar		con_floatZoomStep;
	static idCVar		con_noBackground;

	const idMaterial *	whiteShader;
	const idMaterial *	consoleShader;
};

static idConsoleLocal localConsole;
idConsole	*console = &localConsole;

idCVar idConsoleLocal::con_speed( "con_speed", "3", CVAR_SYSTEM, "speed at which the console moves up and down" );
idCVar idConsoleLocal::con_notifyTime( "con_notifyTime", "3", CVAR_SYSTEM, "time messages are displayed onscreen when console is pulled up" );
#ifdef _DEBUG
idCVar idConsoleLocal::con_noPrint( "con_noPrint", "0", CVAR_BOOL|CVAR_SYSTEM|CVAR_NOCHEAT, "print on the console but not onscreen when console is pulled up" );
#else
idCVar idConsoleLocal::con_noPrint( "con_noPrint", "1", CVAR_BOOL|CVAR_SYSTEM|CVAR_NOCHEAT, "print on the console but not onscreen when console is pulled up" );
#endif
idCVar idConsoleLocal::con_size( "con_size", "0.5", CVAR_SYSTEM | CVAR_FLOAT | CVAR_NOCHEAT | CVAR_ARCHIVE, "screen size of console" );
idCVar idConsoleLocal::con_floating( "con_floating", "0", CVAR_BOOL | CVAR_SYSTEM | CVAR_ARCHIVE, "enable floating console" );
idCVar idConsoleLocal::con_alwaysShow( "con_alwaysShow", "0", CVAR_BOOL | CVAR_SYSTEM | CVAR_ARCHIVE, "text always show when console is closed" );
idCVar idConsoleLocal::con_floatingGeometry("con_floatingGeometry", "100 50 300 200", CVAR_SYSTEM | CVAR_ARCHIVE, "floating console geometry. format is '<left> <top> <width> <height>'" );
idCVar idConsoleLocal::con_floatZoomStep( "con_floatZoomStep", "10", CVAR_INTEGER | CVAR_SYSTEM | CVAR_ARCHIVE, "zoom step of the console in floating mode" );
idCVar idConsoleLocal::con_noBackground( "con_noBackground", "0", CVAR_BOOL | CVAR_SYSTEM | CVAR_ARCHIVE, "don't draw the console background" );

/*
=============================================================================

	Misc stats

=============================================================================
*/

/*
==================
idConsoleLocal::DrawSmallTextLeftAlign
==================
*/
void idConsoleLocal::DrawTextSmallLeftAlign( float x, float &y, const char *text, ... ) {
	char string[MAX_STRING_CHARS];
	va_list argptr;
	va_start( argptr, text );
	idStr::vsnPrintf( string, sizeof( string ), text, argptr );
	va_end( argptr );
	renderSystem->DrawSmallStringExt( x, y + 2, string, colorWhite, true, true, idStr::Length( string ), localConsole.charSetShader );
	y += SMALLCHAR_HEIGHT + 4;
}

/*
==================
idConsoleLocal::DrawTextSmallRightAlign
==================
*/
void idConsoleLocal::DrawTextSmallRightAlign( float x, float &y, const char *text, ... ) {
	char string[MAX_STRING_CHARS];
	va_list argptr;
	va_start( argptr, text );
	int i = idStr::vsnPrintf( string, sizeof( string ), text, argptr );
	va_end( argptr );
	renderSystem->DrawSmallStringExt( x - i * SMALLCHAR_WIDTH, y + 2, string, colorWhite, true, true, idStr::Length( string ), localConsole.charSetShader);
	y += SMALLCHAR_HEIGHT + 4;
}

/*
==================
idConsoleLocal::DrawTextBigLeftAlign
==================
*/
void idConsoleLocal::DrawTextBigLeftAlign( float x, float &y, const char *text, ... ) {
	char string[MAX_STRING_CHARS];
	va_list argptr;
	va_start( argptr, text );
	idStr::vsnPrintf( string, sizeof( string ), text, argptr );
	va_end( argptr );
	renderSystem->DrawBigStringExt( x, y + 2, string, colorWhite, true, true, idStr::Length( string ), localConsole.charSetShader );
	y += BIGCHAR_HEIGHT + 4;
}

/*
==================
idConsoleLocal::DrawTextBigRightAlign
==================
*/
void idConsoleLocal::DrawTextBigRightAlign( float x, float &y, const char *text, ... ) {
	char string[MAX_STRING_CHARS];
	va_list argptr;
	va_start( argptr, text );
	int i = idStr::vsnPrintf( string, sizeof( string ), text, argptr );
	va_end( argptr );
	renderSystem->DrawBigStringExt( x - i * BIGCHAR_WIDTH, y + 2, string, colorWhite, true, true, idStr::Length( string ), localConsole.charSetShader);
	y += BIGCHAR_HEIGHT + 4;
}

/*
==================
idConsoleLocal::DrawFPS
==================
*/
#define	FPS_FRAMES	6
float idConsoleLocal::DrawFPS( float y ) {
	static int previousTimes[FPS_FRAMES];
	static int index;
	static int previous;

	// don't use serverTime, because that will be drifting to
	// correct for internet lag changes, timescales, timedemos, etc
	int t = Sys_Milliseconds();
	int frameTime = t - previous;
	previous = t;

	previousTimes[index % FPS_FRAMES] = frameTime;
	index++;
	if ( index > FPS_FRAMES ) {
		// average multiple frames together to smooth changes out a bit
		int total = 0;
		for ( int i = 0 ; i < FPS_FRAMES ; i++ ) {
			total += previousTimes[i];
		}
		if ( !total ) {
			total = 1;
		}
		int fps = 1000000 * FPS_FRAMES / total;
		fps = ( fps + 500 ) / 1000;

		const char * s = va( "%ifps", fps );

		static idOverlayHandle handle;
		PrintOverlay( handle, JUSTIFY_RIGHT, s, false, TEXTSIZE_LARGE );
	}

	return y + BIGCHAR_HEIGHT + 4;
}

/*
==================
idConsoleLocal::DrawMemoryUsage
==================
*/
float idConsoleLocal::DrawMemoryUsage( float y ) {
	memoryStats_t allocs, frees;

	Mem_GetStats( allocs );
	idStr s = va( "total allocated memory: %4d, %4dkB\n", allocs.num, allocs.totalSize >> 10 );

	Mem_GetFrameStats( allocs, frees );
	s += va( "frame alloc: %4d, %4dkB  frame free: %4d, %4dkB", allocs.num, allocs.totalSize>>10, frees.num, frees.totalSize>>10 );

	Mem_ClearFrameStats();

	static idOverlayHandle handle;
	PrintOverlay( handle, JUSTIFY_LEFT, s.c_str(), false, TEXTSIZE_SMALL );

	return y + BIGCHAR_HEIGHT + 4;
}

/*
==================
idConsoleLocal::DrawAsyncStats
==================
*/
float idConsoleLocal::DrawAsyncStats( float y ) {
	int i, outgoingRate, incomingRate;
	float outgoingCompression, incomingCompression;
	idStr s;

	if ( idAsyncNetwork::server.IsActive() ) {

		s = va( "server delay = %d msec\n", idAsyncNetwork::server.GetDelay() );
		s += va( "total outgoing rate = %d KB/s\n", idAsyncNetwork::server.GetOutgoingRate() >> 10 );
		s += va( "total incoming rate = %d KB/s\n", idAsyncNetwork::server.GetIncomingRate() >> 10 );

		for ( i = 0; i < MAX_ASYNC_CLIENTS; i++ ) {

			outgoingRate = idAsyncNetwork::server.GetClientOutgoingRate( i );
			incomingRate = idAsyncNetwork::server.GetClientIncomingRate( i );
			outgoingCompression = idAsyncNetwork::server.GetClientOutgoingCompression( i );
			incomingCompression = idAsyncNetwork::server.GetClientIncomingCompression( i );

			if ( outgoingRate != -1 && incomingRate != -1 ) {
				s += va( "client %d: out rate = %d B/s (% -2.1f%%), in rate = %d B/s (% -2.1f%%)\n",
								i, outgoingRate, outgoingCompression, incomingRate, incomingCompression );
			}
		}

		idStr msg;
		idAsyncNetwork::server.GetAsyncStatsAvgMsg( msg );
		s += va( "%s", msg.c_str() );

	} else if ( idAsyncNetwork::client.IsActive() ) {

		outgoingRate = idAsyncNetwork::client.GetOutgoingRate();
		incomingRate = idAsyncNetwork::client.GetIncomingRate();
		outgoingCompression = idAsyncNetwork::client.GetOutgoingCompression();
		incomingCompression = idAsyncNetwork::client.GetIncomingCompression();

		if ( outgoingRate != -1 && incomingRate != -1 ) {
			s += va( "out rate = %d B/s (% -2.1f%%), in rate = %d B/s (% -2.1f%%)\n",
							outgoingRate, outgoingCompression, incomingRate, incomingCompression );
		}

		s += va( "packet loss = %d%%, client prediction = %d\n",
							(int)idAsyncNetwork::client.GetIncomingPacketLoss(), idAsyncNetwork::client.GetPrediction() );

		s += va( "predicted frames: %d\n", idAsyncNetwork::client.GetPredictedFrames() );

		static idOverlayHandle handle;
		PrintOverlay( handle, JUSTIFY_LEFT, s.c_str(), false, TEXTSIZE_SMALL );
	}

	return y + BIGCHAR_HEIGHT + 4;
}

/*
==================
idConsoleLocal::DrawSoundDecoders
==================
*/
float idConsoleLocal::DrawSoundDecoders( float y ) {
	const char* decoder;
	int index, numActiveDecoders;
	soundDecoderInfo_t decoderInfo;

	index = -1;
	numActiveDecoders = 0;
	while( ( index = soundSystem->GetSoundDecoderInfo( index, decoderInfo ) ) != -1 ) {
		int localTime = decoderInfo.current44kHzTime - decoderInfo.start44kHzTime;
		int sampleTime = decoderInfo.num44kHzSamples / decoderInfo.numChannels;
		int percent;
		if ( localTime > sampleTime ) {
			if ( decoderInfo.looping ) {
				percent = ( localTime % sampleTime ) * 100 / sampleTime;
			} else {
				percent = 100;
			}
		} else {
			percent = localTime * 100 / sampleTime;
		}
		decoder = va( "%3d: %3d%% (%1.2f) %s: %s (%dkB)", numActiveDecoders, percent, decoderInfo.lastVolume, decoderInfo.format.c_str(), decoderInfo.name.c_str(), decoderInfo.numBytes >> 10 );

		static idOverlayHandle handle;
		PrintOverlay( handle, JUSTIFY_LEFT, decoder, false, TEXTSIZE_SMALL );

		numActiveDecoders++;
	}
	return y + BIGCHAR_HEIGHT + 4;
}

//=========================================================================

/*
==============
Con_Clear_f
==============
*/
static void Con_Clear_f( const idCmdArgs &args ) {
	localConsole.Clear();
}

/*
==============
Con_Dump_f
==============
*/
static void Con_Dump_f( const idCmdArgs &args ) {
	if ( args.Argc() != 2 ) {
		common->Printf( "usage: conDump <filename>\n" );
		return;
	}

	idStr fileName = args.Argv(1);
	fileName.DefaultFileExtension(".txt");

	common->Printf( "Dumped console text to %s.\n", fileName.c_str() );

	localConsole.Dump( fileName.c_str() );
}

/*
==============
idConsoleLocal::Init
==============
*/
void idConsoleLocal::Init( void ) {
	int		i;

	keyCatching = false;

	LOCALSAFE_LEFT		= 32;
	LOCALSAFE_RIGHT		= 608;
	LOCALSAFE_TOP		= 24;
	LOCALSAFE_BOTTOM	= 456;
	LOCALSAFE_WIDTH		= LOCALSAFE_RIGHT - LOCALSAFE_LEFT;
	LOCALSAFE_HEIGHT	= LOCALSAFE_BOTTOM - LOCALSAFE_TOP;

	LINE_WIDTH = ( ( LOCALSAFE_WIDTH / SMALLCHAR_WIDTH ) - 2 );
	TOTAL_LINES = (CON_TEXTSIZE / LINE_WIDTH);

	lastKeyEvent = -1;
	nextKeyEvent = CONSOLE_FIRSTREPEAT;

	consoleField.Clear();

	consoleField.SetWidthInChars( LINE_WIDTH );

	for ( i = 0 ; i < COMMAND_HISTORY ; i++ ) {
		historyEditLines[i].Clear();
		historyEditLines[i].SetWidthInChars( LINE_WIDTH );
	}

	cmdSystem->AddCommand( "clear", Con_Clear_f, CMD_FL_SYSTEM, "clears the console" );
	cmdSystem->AddCommand( "conDump", Con_Dump_f, CMD_FL_SYSTEM, "dumps the console text to a file" );

	float geom[4];


	if( sscanf(  con_floatingGeometry.GetString(), "%f %f %f %f", &geom[0], &geom[1], &geom[2], &geom[3]) == 4 ) {
		consoleX = idMath::FtoiFast(geom[0]);
		consoleY = idMath::FtoiFast(geom[1]);
		consoleW = idMath::FtoiFast(geom[2]);
		consoleH = idMath::FtoiFast(geom[3]);
	} else {
		consoleX = 200;
		consoleY = 50;
		consoleW = 300;
		consoleH = 200;
	}

	drag = false;
	consoleHistory.Init();
}

/*
==============
idConsoleLocal::Shutdown
==============
*/
void idConsoleLocal::Shutdown( void ) {
	cmdSystem->RemoveCommand( "clear" );
	cmdSystem->RemoveCommand( "conDump" );

	consoleHistory.Shutdown();
}

/*
==============
LoadGraphics

Can't be combined with init, because init happens before
the renderSystem is initialized
==============
*/
void idConsoleLocal::LoadGraphics() {
	charSetShader = declManager->FindMaterial( "textures/bigchars" );
	whiteShader = declManager->FindMaterial( "_white" );
	consoleShader = declManager->FindMaterial( "console" );
}

/*
================
idConsoleLocal::Active
================
*/
bool	idConsoleLocal::Active( void ) {
	return keyCatching;
}

/*
================
idConsoleLocal::ClearNotifyLines
================
*/
void	idConsoleLocal::ClearNotifyLines() {
	int		i;

	for ( i = 0 ; i < NUM_CON_TIMES ; i++ ) {
		times[i] = 0;
	}
}

/*
================
idConsoleLocal::Close
================
*/
void	idConsoleLocal::Close() {
	keyCatching = false;
	if( !con_alwaysShow.GetBool() ) {
		SetDisplayFraction( 0 );
		displayFrac = 0;	// don't scroll to that point, go immediately
	}
	ClearNotifyLines();
	drag = false;
}

/*
================
idConsoleLocal::Clear
================
*/
void idConsoleLocal::Clear() {
	int		i;

	for ( i = 0 ; i < CON_TEXTSIZE ; i++ ) {
		text[i] = (idStr::ColorIndex(C_COLOR_CYAN)<<8) | ' ';
	}

	Bottom();		// go to end
}

/*
================
idConsoleLocal::Dump

Save the console contents out to a file
================
*/
void idConsoleLocal::Dump( const char *fileName ) {
	int		l, x, i;
	short *	line;
	idFile *f;
	char	* buffer = (char *)alloca( LINE_WIDTH + 3 );

	f = fileSystem->OpenFileWrite( fileName );
	if ( !f ) {
		common->Warning( "couldn't open %s", fileName );
		return;
	}

	// skip empty lines
	l = current - TOTAL_LINES + 1;
	if ( l < 0 ) {
		l = 0;
	}
	for ( ; l <= current ; l++ )
	{
		line = text + ( l % TOTAL_LINES ) * LINE_WIDTH;
		for ( x = 0; x < LINE_WIDTH; x++ )
			if ( ( line[x] & 0xff ) > ' ' )
				break;
		if ( x != LINE_WIDTH )
			break;
	}

	// write the remaining lines
	for ( ; l <= current; l++ ) {
		line = text + ( l % TOTAL_LINES ) * LINE_WIDTH;
		for( i = 0; i < LINE_WIDTH; i++ ) {
			buffer[i] = line[i] & 0xff;
		}
		for ( x = LINE_WIDTH-1; x >= 0; x-- ) {
			if ( buffer[x] <= ' ' ) {
				buffer[x] = 0;
			} else {
				break;
			}
		}
		buffer[x+1] = '\r';
		buffer[x+2] = '\n';
		buffer[x+3] = 0;
		f->Write( buffer, strlen( buffer ) );
	}

	fileSystem->CloseFile( f );
}

/*
================
idConsoleLocal::PageUp
================
*/
void idConsoleLocal::PageUp( void ) {
	display -= 2;
	if ( current - display >= TOTAL_LINES ) {
		display = current - TOTAL_LINES + 1;
	}
}

/*
================
idConsoleLocal::PageDown
================
*/
void idConsoleLocal::PageDown( void ) {
	display += 2;
	if ( display > current ) {
		display = current;
	}
}

/*
================
idConsoleLocal::Top
================
*/
void idConsoleLocal::Top( void ) {
	display = 0;
}

/*
================
idConsoleLocal::Bottom
================
*/
void idConsoleLocal::Bottom( void ) {
	display = current;
}


/*
=============================================================================

CONSOLE LINE EDITING

==============================================================================
*/

/*
====================
KeyDownEvent

Handles history and console scrollback
====================
*/
void idConsoleLocal::KeyDownEvent( int key ) {

	// Execute F key bindings
	if ( key >= K_F1 && key <= K_F12 ) {
		idKeyInput::ExecKeyBinding( key );
		return;
	}

	// ctrl-L clears screen
	if ( key == 'l' && idKeyInput::IsDown( K_CTRL ) ) {
		Clear();
		return;
	}

	// ctrl-v copy and paste to system clipboard added
	if ( key == 'v' && idKeyInput::IsDown( K_CTRL ) ) {
		consoleField.Paste();
		return;
	}

	// enter finishes the line
	if ( key == K_ENTER || key == K_KP_ENTER ) {

		common->Printf ( "]%s\n", consoleField.GetBuffer() );

		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, consoleField.GetBuffer() );	// valid command
		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "\n" );

		// copy line to history buffer
		if ( consoleField.GetBuffer()[ 0 ] != '\n' && consoleField.GetBuffer()[ 0 ] != '\0' ) {
			consoleHistory.AddToHistory( consoleField.GetBuffer() );
		}

		consoleField.Clear();
		consoleField.SetWidthInChars( LINE_WIDTH );

		session->UpdateScreen();// force an update, because the command
								// may take some time
		return;
	}

	// command completion

	if ( key == K_TAB ) {
		consoleField.AutoComplete();
		return;
	}


	// ctrl-wheel = scale float console
	if( con_floating.GetBool() ) {
		if ( drag || idKeyInput::IsDown( K_CTRL ) ) {
			if ( key == K_MWHEELUP ) {
				ZoomFloatConsole( 1 );
				return;
			} else if ( key == K_MWHEELDOWN ) {
				ZoomFloatConsole( -1 );
				return;
			}
		}
	}

	// command history (ctrl-p ctrl-n for unix style)

	if ( ( key == K_UPARROW ) ||
		 ( ( tolower(key) == 'p' ) && idKeyInput::IsDown( K_CTRL ) ) ) {
		idStr hist = consoleHistory.RetrieveFromHistory( true );
		if ( !hist.IsEmpty() ) {
			consoleField.SetBuffer( hist );
		}
		return;
	}

	if ( ( key == K_DOWNARROW ) ||
		 ( ( tolower( key ) == 'n' ) && idKeyInput::IsDown( K_CTRL ) ) ) {
		idStr hist = consoleHistory.RetrieveFromHistory( false );
		if ( !hist.IsEmpty() ) {
			consoleField.SetBuffer( hist );
		} else {
			consoleField.Clear();	// DG: if no more lines are in the history, show a blank line again
		}
		return;
	}

	// console scrolling
	if ( key == K_PGUP ) {
		PageUp();
		lastKeyEvent = eventLoop->Milliseconds();
		nextKeyEvent = CONSOLE_FIRSTREPEAT;
		return;
	}

	if ( key == K_PGDN ) {
		PageDown();
		lastKeyEvent = eventLoop->Milliseconds();
		nextKeyEvent = CONSOLE_FIRSTREPEAT;
		return;
	}

	if ( key == K_MWHEELUP ) {
		PageUp();
		return;
	}

	if ( key == K_MWHEELDOWN ) {
		PageDown();
		return;
	}

	// ctrl-home = top of console
	if ( key == K_HOME && idKeyInput::IsDown( K_CTRL ) ) {
		Top();
		return;
	}

	// ctrl-end = bottom of console
	if ( key == K_END && idKeyInput::IsDown( K_CTRL ) ) {
		Bottom();
		return;
	}

	// pass to the normal editline routine
	consoleField.KeyDownEvent( key );
}

/*
==============
Scroll
deals with scrolling text because we don't have key repeat
==============
*/
void idConsoleLocal::Scroll( ) {
	if (lastKeyEvent == -1 || (lastKeyEvent+200) > eventLoop->Milliseconds()) {
		return;
	}
	// console scrolling
	if ( idKeyInput::IsDown( K_PGUP ) ) {
		PageUp();
		nextKeyEvent = CONSOLE_REPEAT;
		return;
	}

	if ( idKeyInput::IsDown( K_PGDN ) ) {
		PageDown();
		nextKeyEvent = CONSOLE_REPEAT;
		return;
	}
}

/*
==============
SetDisplayFraction

Causes the console to start opening the desired amount.
==============
*/
void idConsoleLocal::SetDisplayFraction( float frac ) {
	finalFrac = frac;
	fracTime = Sys_Milliseconds();
}

/*
==============
UpdateDisplayFraction

Scrolls the console up or down based on conspeed
==============
*/
void idConsoleLocal::UpdateDisplayFraction( void ) {
	if ( con_speed.GetFloat() <= 0.1f ) {
		fracTime = Sys_Milliseconds();
		displayFrac = finalFrac;
		return;
	}

	// scroll towards the destination height
	if ( finalFrac < displayFrac ) {
		displayFrac -= con_speed.GetFloat() * ( Sys_Milliseconds() - fracTime ) * 0.001f;
		if ( finalFrac > displayFrac ) {
			displayFrac = finalFrac;
		}
		fracTime = Sys_Milliseconds();
	} else if ( finalFrac > displayFrac ) {
		displayFrac += con_speed.GetFloat() * ( Sys_Milliseconds() - fracTime ) * 0.001f;
		if ( finalFrac < displayFrac ) {
			displayFrac = finalFrac;
		}
		fracTime = Sys_Milliseconds();
	}
}

/*
==============
ProcessEvent
==============
*/
bool	idConsoleLocal::ProcessEvent( const sysEvent_t *event, bool forceAccept ) {
	bool consoleKey = false;
	if(event->evType == SE_KEY)
	{
		bool shiftPressed = idKeyInput::IsDown( K_SHIFT );
		if( event->evValue == K_CONSOLE || event->evValue == Sys_GetConsoleKey( shiftPressed )
		   || (event->evValue == K_ESCAPE && shiftPressed) ) // shift+esc should also open console
		{
			consoleKey = true;
		}
	}

#if ID_CONSOLE_LOCK
	// If the console's not already down, and we have it turned off, check for ctrl+alt
	if ( !keyCatching && !com_allowConsole.GetBool() ) {
		if ( !idKeyInput::IsDown( K_CTRL ) || !idKeyInput::IsDown( K_ALT ) ) {
			consoleKey = false;
		}
	}
#endif

	// we always catch the console key event
	if ( !forceAccept && consoleKey ) {
		// ignore up events
		if ( event->evValue2 == 0 ) {
			return true;
		}

		consoleField.ClearAutoComplete();

		// a down event will toggle the destination lines
		if ( keyCatching ) {
			Close();
			cvarSystem->SetCVarBool( "ui_chat", false );
		} else {
			consoleField.Clear();
			keyCatching = true;
			if ( idKeyInput::IsDown( K_SHIFT ) && event->evValue != K_ESCAPE ) {
				// if the shift key is down, don't open the console as much
				// except we used shift+esc.
				SetDisplayFraction( 0.2f );
			} else if ( idKeyInput::IsDown( K_CTRL ) && event->evValue != K_ESCAPE ) {
				// if the ctrl key is down, open the console as much
				// except we used shift+esc.
				SetDisplayFraction( 1.0f );
			} else {
				float consoleSize = con_size.GetFloat();
				if( consoleSize < 0.1f )
				  consoleSize = 0.1f;
				if( consoleSize > 1.0f )
				  consoleSize = 1.0f;

				SetDisplayFraction( consoleSize );
			}
			cvarSystem->SetCVarBool( "ui_chat", true );
		}
		return true;
	}

	// if we aren't key catching, dump all the other events
	if ( !forceAccept && !keyCatching ) {
		return false;
	}

	// handle key and character events
	if ( event->evType == SE_CHAR ) {
		// never send the console key as a character
		if ( event->evValue != Sys_GetConsoleKey( idKeyInput::IsDown( K_SHIFT ) ) ) {
			consoleField.CharEvent( event->evValue );
		}
		return true;
	}

	if ( event->evType == SE_KEY ) {
		// in floating mode, handle dragging
		if ( con_floating.GetBool() && event->evValue == K_MOUSE1 ) {
			drag = event->evValue2 == 1;
		}

		// ignore up key events
		if ( event->evValue2 == 0 ) {
			return true;
		}

		KeyDownEvent( event->evValue );
		return true;
	}

	if ( event->evType == SE_MOUSE ) {
		if( con_floating.GetBool() && drag ) {
			if( event->evValue || event->evValue2 ) {
				consoleX += event->evValue;
				consoleY += event->evValue2;

				con_floatingGeometry.SetString( va( "%d %d %d %d", consoleX, consoleY, consoleW, consoleH ) );
				con_floatingGeometry.ClearModified();
			}
		}
		//return true;
	}

	// we don't handle things like mouse, joystick, and network packets
	return false;
}

/*
==============================================================================

PRINTING

==============================================================================
*/

/*
===============
Linefeed
===============
*/
void idConsoleLocal::Linefeed() {
	int		i;

	// mark time for transparent overlay
	if ( current >= 0 ) {
		times[current % NUM_CON_TIMES] = Sys_Milliseconds();
	}

	x = 0;
	if ( display == current ) {
		display++;
	}
	current++;
	for ( i = 0; i < LINE_WIDTH; i++ ) {
		int offset = ( (unsigned int)current % TOTAL_LINES ) * LINE_WIDTH + i;
		text[offset] = (idStr::ColorIndex(C_COLOR_CYAN)<<8) | ' ';
	}
}


/*
================
Print

Handles cursor positioning, line wrapping, etc
================
*/
void idConsoleLocal::Print( const char *txt ) {
	int		y;
	int		c, l;
	int		color;

#ifdef ID_ALLOW_TOOLS
	RadiantPrint( txt );
#endif

#ifndef IMGUI_DISABLE
	if (com_editors & EDITOR_MATERIAL) {
		ImGuiTools::MaterialEditorPrintConsole(txt);
	}
#elif defined(ID_ALLOW_TOOLS)
	if (com_editors & EDITOR_MATERIAL) {
		MaterialEditorPrintConsole(txt);
	}
#endif

	if ( TOTAL_LINES == 0 ) {
		// not yet initialized
		return;
	}

	color = idStr::ColorIndex( C_COLOR_CYAN );

	while ( (c = *(const unsigned char*)txt) != 0 ) {
		if ( idStr::IsColor( txt ) ) {
			if ( *(txt+1) == C_COLOR_DEFAULT ) {
				color = idStr::ColorIndex( C_COLOR_CYAN );
			} else {
				color = idStr::ColorIndex( *(txt+1) );
			}
			txt += 2;
			continue;
		}

		y = current % TOTAL_LINES;

		// if we are about to print a new word, check to see
		// if we should wrap to the new line
		if ( c > ' ' && ( x == 0 || text[y*LINE_WIDTH+x-1] <= ' ' ) ) {
			// count word length
			for (l=0 ; l< LINE_WIDTH ; l++) {
				if ( txt[l] <= ' ') {
					break;
				}
			}

			// word wrap
			if (l != LINE_WIDTH && (x + l >= LINE_WIDTH) ) {
				Linefeed();
			}
		}

		txt++;

		switch( c ) {
			case '\n':
				Linefeed ();
				break;
			case '\t':
				do {
					text[y*LINE_WIDTH+x] = (color << 8) | ' ';
					x++;
					if ( x >= LINE_WIDTH ) {
						Linefeed();
						x = 0;
					}
				} while ( x & 3 );
				break;
			case '\r':
				x = 0;
				break;
			default:	// display character and advance
				text[y*LINE_WIDTH+x] = (color << 8) | c;
				x++;
				if ( x >= LINE_WIDTH ) {
					Linefeed();
					x = 0;
				}
				break;
		}
	}


	// mark time for transparent overlay
	if ( current >= 0 ) {
		times[current % NUM_CON_TIMES] = Sys_Milliseconds();
	}
}


/*
==============================================================================

DRAWING

==============================================================================
*/


/*
================
DrawInput

Draw the editline after a ] prompt
================
*/
void idConsoleLocal::DrawInput() {
	int y, autoCompleteLength;

	y = vislines - ( SMALLCHAR_HEIGHT * 2 );

	if ( consoleField.GetAutoCompleteLength() != 0 ) {
		autoCompleteLength = strlen( consoleField.GetBuffer() ) - consoleField.GetAutoCompleteLength();

		if ( autoCompleteLength > 0 ) {
			renderSystem->SetColor4( .8f, .2f, .2f, .45f );
			renderSystem->DrawStretchPic( LOCALSAFE_LEFT + 2 * SMALLCHAR_WIDTH + consoleField.GetAutoCompleteLength() * SMALLCHAR_WIDTH,
							y + 2, autoCompleteLength * SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT - 2, 0, 0, 0, 0, whiteShader );
		}
	}

	renderSystem->SetColor( idStr::ColorForIndex( C_COLOR_CYAN ) );

	renderSystem->DrawSmallChar( LOCALSAFE_LEFT + 1 * SMALLCHAR_WIDTH, y, ']', localConsole.charSetShader );

	consoleField.Draw( LOCALSAFE_LEFT + 2 * SMALLCHAR_WIDTH, y, SCREEN_WIDTH - 3 * SMALLCHAR_WIDTH, true, charSetShader );
}


/*
================
DrawNotify

Draws the last few lines of output transparently over the game top
================
*/
void idConsoleLocal::DrawNotify() {
	int		x, v;
	short	*text_p;
	int		i;
	int		time;
	int		currentColor;

	if ( con_noPrint.GetBool() ) {
		return;
	}

	currentColor = idStr::ColorIndex( C_COLOR_WHITE );
	renderSystem->SetColor( idStr::ColorForIndex( currentColor ) );

	v = 0;
	for ( i = current-NUM_CON_TIMES+1; i <= current; i++ ) {
		if ( i < 0 ) {
			continue;
		}
		time = times[i % NUM_CON_TIMES];
		if ( time == 0 ) {
			continue;
		}
		time = Sys_Milliseconds() - time;
		if ( time > con_notifyTime.GetFloat() * 1000 ) {
			continue;
		}
		text_p = text + (i % TOTAL_LINES)*LINE_WIDTH;

		for ( x = 0; x < LINE_WIDTH; x++ ) {
			if ( ( text_p[x] & 0xff ) == ' ' ) {
				continue;
			}
			if ( idStr::ColorIndex(text_p[x]>>8) != currentColor ) {
				currentColor = idStr::ColorIndex(text_p[x]>>8);
				renderSystem->SetColor( idStr::ColorForIndex( currentColor ) );
			}
			//renderSystem->DrawSmallChar( LOCALSAFE_LEFT + (x+1)*SMALLCHAR_WIDTH, v, text_p[x] & 0xff, localConsole.charSetShader );

			int length = idStr::Length( va( "%c", text_p[x] & 0xff ) );
			renderSystem->DrawSmallStringExt( LOCALSAFE_LEFT + (x+1)*SMALLCHAR_WIDTH, v, va( "%c", text_p[x] & 0xff ), idStr::ColorForIndex( currentColor ), false, true, length, localConsole.charSetShader);
		}

		v += SMALLCHAR_HEIGHT;
	}

	renderSystem->SetColor( colorCyan );
}

/*
================
DrawSolidConsole

Draws the console with the solid background
================
*/
void idConsoleLocal::DrawSolidConsole( float frac ) {
	int				i, x;
	float			y;
	int				rows;
	short			*text_p;
	int				row;
	int				lines;
	int				currentColor;

	if( frac < 1.0f && con_floating.GetBool() ) {
		DrawFloatConsole();
		return;
	}

	lines = idMath::Ftoi( SCREEN_HEIGHT * frac );
	if ( lines <= 0 ) {
		return;
	}

	if ( lines > SCREEN_HEIGHT ) {
		lines = SCREEN_HEIGHT;
	}

	// draw the background
	y = frac * SCREEN_HEIGHT - 2;
	if ( y < 1.0f ) {
		y = 0.0f;
	} else {
		if ( ( !con_alwaysShow.GetBool() || keyCatching ) && !con_noBackground.GetBool() ) {
			renderSystem->DrawStretchPic( 0, 0, SCREEN_WIDTH, y, 0, 1.0f - displayFrac, 1, 1, consoleShader );
		}
	}

	if ( !con_alwaysShow.GetBool() || keyCatching ) {
		renderSystem->SetColor( colorCyan );
		renderSystem->DrawStretchPic( 0, y, SCREEN_WIDTH, 2, 0, 0, 0, 0, whiteShader );
		renderSystem->SetColor( colorCyan );
	}

	// draw the version number
	if( !con_alwaysShow.GetBool() || keyCatching ) {
		renderSystem->SetColor( idStr::ColorForIndex( C_COLOR_CYAN ) );

#ifdef _MSC_VER
		idStr version = va( "%s.%i %s-%s", ENGINE_VERSION, BUILD_NUMBER, BUILD_OS, D3_ARCH );
#else
		idStr version = va( "%s.%i %s-%s", ENGINE_VERSION, BUILD_NUMBER, BUILD_OS, BUILD_CPU );
#endif // _MSC_VER
		i = version.Length();

		for ( x = 0; x < i; x++ ) {
			renderSystem->DrawSmallChar( LOCALSAFE_WIDTH - ( i - x ) * SMALLCHAR_WIDTH, 
				(lines-(SMALLCHAR_HEIGHT+SMALLCHAR_HEIGHT/4)), version[x], localConsole.charSetShader );

		}
	}


	// draw the text
	vislines = lines;
	rows = (lines-SMALLCHAR_WIDTH)/SMALLCHAR_WIDTH;		// rows of text to draw

	y = lines - (SMALLCHAR_HEIGHT*3);

	// draw from the bottom up
	if ( display != current ) {
		// draw arrows to show the buffer is backscrolled
		renderSystem->SetColor( idStr::ColorForIndex( C_COLOR_CYAN ) );
		for ( x = 0; x < LINE_WIDTH; x += 4 ) {
			renderSystem->DrawSmallChar( LOCALSAFE_LEFT + (x+1)*SMALLCHAR_WIDTH, idMath::Ftoi( y ), '^', localConsole.charSetShader );
		}
		y -= SMALLCHAR_HEIGHT;
		rows--;
	}

	row = display;

	if ( x == 0 ) {
		row--;
	}

	currentColor = idStr::ColorIndex( C_COLOR_WHITE );
	renderSystem->SetColor( idStr::ColorForIndex( currentColor ) );

	for ( i = 0; i < rows; i++, y -= SMALLCHAR_HEIGHT, row-- ) {
		if ( row < 0 ) {
			break;
		}
		if ( current - row >= TOTAL_LINES ) {
			// past scrollback wrap point
			continue;
		}

		text_p = text + (row % TOTAL_LINES)*LINE_WIDTH;

		for ( x = 0; x < LINE_WIDTH; x++ ) {
			if ( ( text_p[x] & 0xff ) == ' ' ) {
				continue;
			}

			if ( idStr::ColorIndex(text_p[x]>>8) != currentColor ) {
				currentColor = idStr::ColorIndex(text_p[x]>>8);
				renderSystem->SetColor( idStr::ColorForIndex( currentColor ) );
			}
			renderSystem->DrawSmallChar( LOCALSAFE_LEFT + (x+1)*SMALLCHAR_WIDTH, idMath::Ftoi( y ), text_p[x] & 0xff, localConsole.charSetShader );
		}
	}

	// draw the input prompt, user text, and cursor if desired
	if ( !con_alwaysShow.GetBool() || keyCatching ) {
		DrawInput();
	}

	renderSystem->SetColor( colorCyan );
}

/*
================
idConsoleLocal::ZoomFloatConsole
================
*/
void idConsoleLocal::ZoomFloatConsole( int inOut ) {
	int step = con_floatZoomStep.GetInteger() / 2 * inOut;
	if ( step == 0 ) {
		return;
	}

	consoleX -= step;
	consoleY -= step;
	consoleW += step * 2;
	consoleH += step * 2;

	con_floatingGeometry.SetString( va( "%d %d %d %d", consoleX, consoleY, consoleW, consoleH ) );
	con_floatingGeometry.ClearModified();
}

/*
================
idConsoleLocal::DrawFloatConsole

Draws the console with the float background
================
*/
void idConsoleLocal::DrawFloatConsole( void ) {
	int				i, x;
	float			y;
	int				rows;
	short			*text_p;
	int				row;
	int				lines;
	int				currentColor;

	if ( !con_floating.GetBool() ) {
		return;
	}

	if ( con_floatingGeometry.IsModified() ) {
		float geom[4];

		if( sscanf( con_floatingGeometry.GetString(), "%f %f %f %f", &geom[0], &geom[1], &geom[2], &geom[3] ) == 4 ) {
			consoleX = idMath::FtoiFast( geom[0] );
			consoleY = idMath::FtoiFast( geom[1] );
			consoleW = idMath::FtoiFast( geom[2] );
			consoleH = idMath::FtoiFast( geom[3] );
		}

		con_floatingGeometry.ClearModified();
	}

	lines = consoleH;

	if ( lines <= 0 ) {
		return;
	}

	const int lineWidth = idMath::FtoiFast( (float)consoleW * (float)LINE_WIDTH / (float)SCREEN_WIDTH );
	const int totalLines = CON_TEXTSIZE / lineWidth;

	if ( lines > SCREEN_HEIGHT ) {
		lines = SCREEN_HEIGHT;
	}

	// draw the background
	y = consoleH - 2;

	if ( y < 1.0f ) {
		y = 0.0f;
	} else {
		if( ( !con_alwaysShow.GetBool() || keyCatching) && !drag && !con_noBackground.GetBool() ) {
			renderSystem->DrawStretchPic( consoleX, consoleY, consoleW, y, 0, 1.0f - displayFrac, 1, 1, consoleShader );
		}

	}

	if ( ( !con_alwaysShow.GetBool() || keyCatching ) && !drag ) {
		renderSystem->SetColor( colorCyan );
		renderSystem->DrawStretchPic( consoleX, y + consoleY, consoleW, 2, 0, 0, 0, 0, whiteShader );
	}

	renderSystem->SetColor( colorWhite );

	// draw the version number
	if( !con_alwaysShow.GetBool() || keyCatching ) {
		renderSystem->SetColor( idStr::ColorForIndex( C_COLOR_CYAN ) );

#ifdef _MSC_VER
		idStr version = va( "%s.%i %s-%s", ENGINE_VERSION, BUILD_NUMBER, BUILD_OS, D3_ARCH );
#else
		idStr version = va( "%s.%i %s-%s", ENGINE_VERSION, BUILD_NUMBER, BUILD_OS, BUILD_CPU );
#endif // _MSC_VER
		i = version.Length();

		for ( x = 0; x < i; x++ ) {
			renderSystem->DrawSmallChar( consoleW - (i - x) * SMALLCHAR_WIDTH + consoleX,
					(lines-(SMALLCHAR_HEIGHT+SMALLCHAR_HEIGHT/2)) + consoleY, version[x], localConsole.charSetShader );
		}
	}

	// draw the text
	vislines = lines;

	rows = (lines-SMALLCHAR_WIDTH)/SMALLCHAR_WIDTH;		// rows of text to draw

	y = lines - (SMALLCHAR_HEIGHT*3);

	// draw from the bottom up
	if ( display != current ) {
		// draw arrows to show the buffer is backscrolled
		renderSystem->SetColor( idStr::ColorForIndex( C_COLOR_CYAN ) );

		for ( x = 0; x < lineWidth; x += 4 ) {
			renderSystem->DrawSmallChar( ( x + 1 ) * SMALLCHAR_WIDTH + consoleX, idMath::FtoiFast(y) + consoleY, '^', localConsole.charSetShader );
		}

		y -= SMALLCHAR_HEIGHT;

		rows--;
	}

	row = display;

	if ( x == 0 ) {
		row--;
	}

	currentColor = idStr::ColorIndex( C_COLOR_WHITE );
	renderSystem->SetColor( idStr::ColorForIndex( currentColor ) );

	for ( i = 0; i < rows; i++, row-- ) {
		if ( row < 0 ) {
			break;
		}

		if ( current - row >= TOTAL_LINES ) {
			// past scrollback wrap point
			continue;
		}

		text_p = text + (row % TOTAL_LINES)*LINE_WIDTH;

		int chars;

		for( chars = LINE_WIDTH - 1; chars >= 0; chars-- ) {
			if ( ( text_p[chars] & 0xff) != ' ' )
				break;
		}

		chars++;

		int nl = chars / lineWidth + (chars % lineWidth ? 1 : 0);
		int charsHeight = nl * SMALLCHAR_HEIGHT;
		int baseY = y - charsHeight;

		if ( y - charsHeight < 0 ) {
			break;
		}

		int nx = 0;

		for ( x = 0; x < chars; x++ ) {
			if ( ( text_p[x] & 0xff ) != ' ' ) {


				if ( idStr::ColorIndex( text_p[x] >> 8 ) != currentColor ) {
					currentColor = idStr::ColorIndex( text_p[x] >> 8 );
					renderSystem->SetColor( idStr::ColorForIndex( currentColor ) );
				}

				renderSystem->DrawSmallChar( (nx + 1 ) * SMALLCHAR_WIDTH + consoleX, idMath::FtoiFast(baseY) + consoleY, text_p[x] & 0xff, localConsole.charSetShader);
			}

			if( nx >= lineWidth ) {
				nx = 0;

				baseY += SMALLCHAR_HEIGHT;
			}
			else
				nx++;


		}

		y -= charsHeight;
	}

	// draw the input prompt, user text, and cursor if desired
	if( !con_alwaysShow.GetBool() || keyCatching ) {
		int y, autoCompleteLength;

		y = vislines - (SMALLCHAR_HEIGHT * 2) + consoleY;

		if ( consoleField.GetAutoCompleteLength() != 0 ) {
			autoCompleteLength = strlen( consoleField.GetBuffer() ) - consoleField.GetAutoCompleteLength();

			if ( autoCompleteLength > 0 ) {
				renderSystem->SetColor4( .8f, .2f, .2f, .45f );
				renderSystem->DrawStretchPic( 2 * SMALLCHAR_WIDTH + consoleField.GetAutoCompleteLength() * SMALLCHAR_WIDTH + consoleX,
						y + 2, autoCompleteLength * SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT - 2, 0, 0, 0, 0, whiteShader );
			}
		}

		renderSystem->SetColor( idStr::ColorForIndex( C_COLOR_CYAN ) );
		renderSystem->DrawSmallChar( 1 * SMALLCHAR_WIDTH + consoleX, y, ']', localConsole.charSetShader );

		consoleField.Draw( 2 * SMALLCHAR_WIDTH + consoleX, y, SCREEN_WIDTH - 3 * SMALLCHAR_WIDTH, true, charSetShader );
	}

	renderSystem->SetColor( colorCyan );
}

/*
==============
Draw

ForceFullScreen is used by the editor
==============
*/
void	idConsoleLocal::Draw( bool forceFullScreen ) {
	float y = 0.0f;

	if ( !charSetShader ) {
		return;
	}

	if ( forceFullScreen ) {
		// if we are forced full screen because of a disconnect,
		// we want the console closed when we go back to a session state
		Close();
		// we are however catching keyboard input
		keyCatching = true;
	}

	Scroll();

	UpdateDisplayFraction();

	if ( forceFullScreen ) {
		DrawSolidConsole( 1.0f );
	} else if ( displayFrac ) {
		DrawSolidConsole( displayFrac );
	} else {
		// only draw the notify lines if the developer cvar is set,
		// or we are a debug build
		if ( !con_noPrint.GetBool() ) {
			DrawNotify();
		}
	}

	float lefty = LOCALSAFE_TOP;
	float righty = LOCALSAFE_TOP;
	float centery = LOCALSAFE_TOP;
	if ( com_showFPS.GetBool() ) {
		righty = DrawFPS( righty );
	}
	if ( com_showMemoryUsage.GetBool() ) {
		righty = DrawMemoryUsage( righty );
	}
	if ( com_showAsyncStats.GetBool() ) {
		righty = DrawAsyncStats( y );
	}
	if ( com_showSoundDecoders.GetBool() ) {
		righty = DrawSoundDecoders( y );
	}
	DrawOverlayText( lefty, righty, centery );
}

/*
========================
idConsoleLocal::PrintOverlay 
========================
*/
void idConsoleLocal::PrintOverlay( idOverlayHandle &handle, justify_t justify, const char *text, bool showbackground, textSize_t size, ... ) {
	if ( handle.index >= 0 && handle.index < overlayText.Num() ) {
		if ( overlayText[handle.index].time == handle.time ) {
			return;
		}
	}

	char string[MAX_PRINT_MSG];
	va_list argptr;
	va_start( argptr, size );
	idStr::vsnPrintf( string, sizeof( string ), text, argptr );
	va_end( argptr );

	overlayText_t &overlay = overlayText.Alloc();
	overlay.text = string;
	overlay.justify = justify;
	overlay.size = size;
	overlay.time = Sys_Milliseconds();
	overlay.showbackground = showbackground;

	handle.index = overlayText.Num() - 1;
	handle.time = overlay.time;
}

/*
========================
idConsoleLocal::DrawOverlayText
========================
*/
void idConsoleLocal::DrawOverlayText( float & leftY, float & rightY, float & centerY ) {
	for ( int i = 0; i < overlayText.Num(); i++ ) {
		const idStr & text = overlayText[i].text;

		int maxWidth = 0;
		int numLines = 0;
		for ( int j = 0; j < text.Length(); j++ ) {
			int width = 1;
			for (; j < text.Length() && text[j] != '\n'; j++ ) {
				width++;
			}
			numLines++;
			if ( width > maxWidth ) {
				maxWidth = width;
			}
		}

		const float width = maxWidth * SMALLCHAR_WIDTH;
		const float height = numLines * ( SMALLCHAR_HEIGHT + 4 );

		if ( overlayText[i].showbackground ) {
			idVec4 bgColor( 0.0f, 0.0f, 0.0f, 0.75f );

			const float bgAdjust = - 0.5f * SMALLCHAR_WIDTH;
			if ( overlayText[i].justify == JUSTIFY_LEFT ) {
				renderSystem->SetColor( bgColor );
				renderSystem->DrawStretchPic( LOCALSAFE_LEFT + bgAdjust, leftY, width, height, 0, 0, 0, 0, whiteShader );
				renderSystem->SetColor( bgColor );
			} else if ( overlayText[i].justify == JUSTIFY_RIGHT ) {
				renderSystem->SetColor( bgColor );
				renderSystem->DrawStretchPic( LOCALSAFE_RIGHT - width + bgAdjust, rightY, width, height, 0, 0, 0, 0, whiteShader );
				renderSystem->SetColor( bgColor );
			} else if ( overlayText[i].justify == JUSTIFY_CENTER_LEFT || overlayText[i].justify == JUSTIFY_CENTER_RIGHT ) {
				renderSystem->SetColor( bgColor );
				renderSystem->DrawStretchPic( LOCALSAFE_LEFT + ( LOCALSAFE_WIDTH - width + bgAdjust ) * 0.5f, centerY, width, height, 0, 0, 0, 0, whiteShader );
				renderSystem->SetColor( bgColor );
			} else {
				assert( false );
			}
		}

		idStr singleLine;
		for ( int j = 0; j < text.Length(); j += singleLine.Length() + 1 ) {
			singleLine = "";
			for ( int k = j; k < text.Length() && text[k] != '\n'; k++ ) {
				singleLine.Append( text[k] );
			}
			if ( overlayText[i].size == TEXTSIZE_SMALL ) {
				if ( overlayText[i].justify == JUSTIFY_LEFT ) {
					DrawTextSmallLeftAlign( LOCALSAFE_LEFT, leftY, "%s", singleLine.c_str() );
				} else if ( overlayText[i].justify == JUSTIFY_RIGHT ) {
					DrawTextSmallRightAlign( LOCALSAFE_RIGHT, rightY, "%s", singleLine.c_str() );
				} else if ( overlayText[i].justify == JUSTIFY_CENTER_LEFT ) {
					DrawTextSmallLeftAlign( LOCALSAFE_LEFT + ( LOCALSAFE_WIDTH - width ) * 0.5f, centerY, "%s", singleLine.c_str() );
				} else if ( overlayText[i].justify == JUSTIFY_CENTER_RIGHT ) {
					DrawTextSmallRightAlign( LOCALSAFE_LEFT + ( LOCALSAFE_WIDTH + width ) * 0.5f, centerY, "%s", singleLine.c_str() );
				} else {
					assert( false );
				}	
			} else {
				if ( overlayText[i].justify == JUSTIFY_LEFT ) {
					DrawTextBigLeftAlign( LOCALSAFE_LEFT, leftY, "%s", singleLine.c_str() );
				} else if ( overlayText[i].justify == JUSTIFY_RIGHT ) {
					DrawTextBigRightAlign( LOCALSAFE_RIGHT, rightY, "%s", singleLine.c_str() );
				} else if ( overlayText[i].justify == JUSTIFY_CENTER_LEFT ) {
					DrawTextBigLeftAlign( LOCALSAFE_LEFT + ( LOCALSAFE_WIDTH - width ) * 0.5f, centerY, "%s", singleLine.c_str() );
				} else if ( overlayText[i].justify == JUSTIFY_CENTER_RIGHT ) {
					DrawTextBigRightAlign( LOCALSAFE_LEFT + ( LOCALSAFE_WIDTH + width ) * 0.5f, centerY, "%s", singleLine.c_str() );
				} else {
					assert( false );
				}	
			}
		}
	}
	overlayText.SetNum( 0 );
}