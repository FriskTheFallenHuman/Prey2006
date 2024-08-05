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
#include "Session_local.h"

/*

New for tech4x:

Unlike previous SMP work, the actual GPU command drawing is done in the main thread, which avoids the
OpenGL problems with needing windows to be created by the same thread that creates the context, as well
as the issues with passing context ownership back and forth on the 360.

The game tic and the generation of the draw command list is now run in a separate thread, and overlapped
with the interpretation of the previous draw command list.

While the game tic should be nicely contained, the draw command generation winds through the user interface
code, and is potentially hazardous.  For now, the overlap will be restricted to the renderer back end,
which should also be nicely contained.

*/
#define DEFAULT_FIXED_TIC "0"
#define DEFAULT_NO_SLEEP "0"

idCVar com_deltaTimeClamp( "com_deltaTimeClamp", "50", CVAR_INTEGER, "don't process more than this time in a single frame" );

idCVar	idSessionLocal::com_fixedTic( "com_fixedTic", DEFAULT_FIXED_TIC, CVAR_SYSTEM | CVAR_INTEGER | CVAR_ARCHIVE, "", -1, 10 );
//idCVar com_fixedTic( "com_fixedTic", DEFAULT_FIXED_TIC, CVAR_BOOL, "run a single game frame per render frame" );
//idCVar com_noSleep( "com_noSleep", DEFAULT_NO_SLEEP, CVAR_BOOL, "don't sleep if the game is running too fast" );
//idCVar com_smp( "com_smp", "1", CVAR_BOOL|CVAR_SYSTEM|CVAR_NOCHEAT, "run the game and draw code in a separate thread" );
idCVar	idSessionLocal::com_aviDemoSamples( "com_aviDemoSamples", "16", CVAR_SYSTEM, "" );
idCVar	idSessionLocal::com_aviDemoWidth( "com_aviDemoWidth", "256", CVAR_SYSTEM, "" );
idCVar	idSessionLocal::com_aviDemoHeight( "com_aviDemoHeight", "256", CVAR_SYSTEM, "" );
idCVar	idSessionLocal::com_aviDemoTics( "com_aviDemoTics", "2", CVAR_SYSTEM | CVAR_INTEGER, "", 1, 60 );
idCVar	idSessionLocal::com_skipGameDraw( "com_skipGameDraw", "0", CVAR_SYSTEM | CVAR_BOOL, "" );
//idCVar com_aviDemoSamples( "com_aviDemoSamples", "16", CVAR_SYSTEM, "" );
//idCVar com_aviDemoWidth( "com_aviDemoWidth", "256", CVAR_SYSTEM, "" );
//idCVar com_aviDemoHeight( "com_aviDemoHeight", "256", CVAR_SYSTEM, "" );
//idCVar com_skipGameDraw( "com_skipGameDraw", "0", CVAR_SYSTEM | CVAR_BOOL, "" );

//idCVar com_sleepGame( "com_sleepGame", "0", CVAR_SYSTEM | CVAR_INTEGER, "intentionally add a sleep in the game time" );
//idCVar com_sleepDraw( "com_sleepDraw", "0", CVAR_SYSTEM | CVAR_INTEGER, "intentionally add a sleep in the draw time" );
//idCVar com_sleepRender( "com_sleepRender", "0", CVAR_SYSTEM | CVAR_INTEGER, "intentionally add a sleep in the render time" );

//idCVar net_drawDebugHud( "net_drawDebugHud", "0", CVAR_SYSTEM | CVAR_INTEGER, "0 = None, 1 = Hud 1, 2 = Hud 2, 3 = Snapshots" );

idCVar com_timescale( "timescale", "1", CVAR_SYSTEM | CVAR_FLOAT, "scales the time", 0.1f, 10.0f );
//idCVar timescale( "timescale", "1", CVAR_SYSTEM | CVAR_FLOAT, "Number of game frames to run per render frame", 0.001f, 100.0f );

/*
===============
idSessionLocal::DrawWipeModel

Draw the fade material over everything that has been drawn
===============
*/
void idSessionLocal::DrawWipeModel() {
	int latchedTic = com_ticNumber;

	if ( wipeStartTic >= wipeStopTic ) {
		return;
	}

	if ( !wipeHold && latchedTic >= wipeStopTic ) {
		return;
	}

	float fade = ( float )( latchedTic - wipeStartTic ) / ( wipeStopTic - wipeStartTic );
	renderSystem->SetColor4( 1, 1, 1, fade );
	renderSystem->DrawStretchPic( 0, 0, 640, 480, 0, 0, 1, 1, wipeMaterial );
}

/*
===============
idSessionLocal::DrawCmdGraph

Graphs yaw angle for testing smoothness
===============
*/
static const int	ANGLE_GRAPH_HEIGHT = 128;
static const int	ANGLE_GRAPH_STRETCH = 3;
void idSessionLocal::DrawCmdGraph() {
	if ( !com_showAngles.GetBool() ) {
		return;
	}
	renderSystem->SetColor4( 0.1f, 0.1f, 0.1f, 1.0f );
	renderSystem->DrawStretchPic( 0, 480-ANGLE_GRAPH_HEIGHT, MAX_BUFFERED_USERCMD*ANGLE_GRAPH_STRETCH, ANGLE_GRAPH_HEIGHT, 0, 0, 1, 1, whiteMaterial );
	renderSystem->SetColor4( 0.9f, 0.9f, 0.9f, 1.0f );
	for ( int i = 0 ; i < MAX_BUFFERED_USERCMD-4 ; i++ ) {
		usercmd_t	cmd = usercmdGen->TicCmd( latchedTicNumber - (MAX_BUFFERED_USERCMD-4) + i );
		int h = cmd.angles[1];
		h >>= 8;
		h &= (ANGLE_GRAPH_HEIGHT-1);
		renderSystem->DrawStretchPic( i* ANGLE_GRAPH_STRETCH, 480-h, 1, h, 0, 0, 1, 1, whiteMaterial );
	}
}

/*
===============
idSessionLocal::Draw
===============
*/
void idSessionLocal::Draw() {
	bool fullConsole = false;

	if ( insideExecuteMapChange ) {
		if ( guiLoading ) {
			guiLoading->Redraw( com_frameTime );
		}
		if ( guiActive == guiMsg ) {
			guiMsg->Redraw( com_frameTime );
		}
	} else if ( guiTest ) {
		// if testing a gui, clear the screen and draw it
		// clear the background, in case the tested gui is transparent
		// NOTE that you can't use this for aviGame recording, it will tick at real com_frameTime between screenshots..
		renderSystem->SetColor( colorBlack );
		renderSystem->DrawStretchPic( 0, 0, 640, 480, 0, 0, 1, 1, declManager->FindMaterial( "_white" ) );
		guiTest->Redraw( com_frameTime );
	} else if ( guiActive && !guiActive->State().GetBool( "gameDraw" ) ) {

		// draw the frozen gui in the background
		if ( guiActive == guiMsg && guiMsgRestore ) {
			guiMsgRestore->Redraw( com_frameTime );
		}

		// draw the menus full screen
		if ( !com_skipGameDraw.GetBool() ) {
			game->Draw( GetLocalClientNum() );
			if ( guiSubtitles ) {
				guiSubtitles->Redraw( com_frameTime );
			}
		}

		guiActive->Redraw( com_frameTime );
	} else if ( readDemo ) {
		rw->RenderScene( &currentDemoRenderView );
		renderSystem->DrawDemoPics();
	} else if ( mapSpawned ) {
		bool gameDraw = false;
		// normal drawing for both single and multi player
		if ( !com_skipGameDraw.GetBool() && GetLocalClientNum() >= 0 ) {
			// draw the game view
			int	start = Sys_Milliseconds();
			gameDraw = game->Draw( GetLocalClientNum() );
			if ( guiSubtitles ) {
				guiSubtitles->Redraw( com_frameTime );
			}
			int end = Sys_Milliseconds();
			time_gameDraw += ( end - start );	// note time used for com_speeds
		}
		if ( !gameDraw ) {
			renderSystem->SetColor( colorBlack );
			renderSystem->DrawStretchPic( 0, 0, 640, 480, 0, 0, 1, 1, declManager->FindMaterial( "_white" ) );
		}

		// save off the 2D drawing from the game
		if ( writeDemo ) {
			renderSystem->WriteDemoPics();
		}
	} else {
#if ID_CONSOLE_LOCK
		if ( com_allowConsole.GetBool() ) {
			console->Draw( true );
		} else {
			emptyDrawCount++;
			if ( emptyDrawCount > 5 ) {
				// it's best if you can avoid triggering the watchgod by doing the right thing somewhere else
				assert( false );
				common->Warning( "idSession: triggering mainmenu watchdog" );
				emptyDrawCount = 0;
				StartMenu();
			}
			renderSystem->SetColor4( 0, 0, 0, 1 );
			renderSystem->DrawStretchPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, 1, 1, declManager->FindMaterial( "_white" ) );
		}
#else
		// draw the console full screen - this should only ever happen in developer builds
		console->Draw( true );
#endif
		fullConsole = true;
	}

#if ID_CONSOLE_LOCK
	if ( !fullConsole && emptyDrawCount ) {
		common->DPrintf( "idSession: %d empty frame draws\n", emptyDrawCount );
		emptyDrawCount = 0;
	}
	fullConsole = false;
#endif

	// draw the wipe material on top of this if it hasn't completed yet
	DrawWipeModel();

	// draw debug graphs
	DrawCmdGraph();

	// draw the half console / notify console on top of everything
	if ( !fullConsole ) {
		console->Draw( false );
	}
}

/*
===============
idSessionLocal::UpdateScreen
===============
*/
void idSessionLocal::UpdateScreen( bool outOfSequence ) {

#ifdef _WIN32

	if ( com_editors ) {
		if ( !Sys_IsWindowVisible() ) {
			return;
		}
	}
#endif

	if ( insideUpdateScreen ) {
		return;
//		common->FatalError( "idSessionLocal::UpdateScreen: recursively called" );
	}

	insideUpdateScreen = true;

	// if this is a long-operation update and we are in windowed mode,
	// release the mouse capture back to the desktop
	if ( outOfSequence ) {
		Sys_GrabMouseCursor( false );
	}

	renderSystem->BeginFrame( renderSystem->GetScreenWidth(), renderSystem->GetScreenHeight() );

	// draw everything
	Draw();

	if ( com_speeds.GetBool() ) {
		renderSystem->EndFrame( &time_frontend, &time_backend );
	} else {
		renderSystem->EndFrame( NULL, NULL );
	}

	insideUpdateScreen = false;
}

extern int com_frameNumber;		// variable frame number
extern idCVar com_forceGenericSIMD;

/*
=================
idCommonLocal::Frame
=================
*/
void idCommonLocal::Frame( void ) {
	try {

		// pump all the events
		Sys_GenerateEvents();

		// write config file if anything changed
		WriteConfiguration();

		// change SIMD implementation if required
		if ( com_forceGenericSIMD.IsModified() ) {
			InitSIMD();
		}

		if ( com_enableDebuggerServer.IsModified() ) {
			if ( com_enableDebuggerServer.GetBool() ) {
				DebuggerServerInit();
			} else {
				DebuggerServerShutdown();
			}
		}

		eventLoop->RunEventLoop();

		com_frameTime = com_ticNumber * USERCMD_MSEC;

		idAsyncNetwork::RunFrame();

		if ( idAsyncNetwork::IsActive() ) {
			if ( idAsyncNetwork::serverDedicated.GetInteger() != 1 ) {
				session->GuiFrameEvents();
				session->UpdateScreen( false );
			}
		} else {
			session->Frame();

			// normal, in-sequence screen update
			session->UpdateScreen( false );
		}

		// report timing information
		if ( com_speeds.GetBool() ) {
			static int	lastTime;
			int		nowTime = Sys_Milliseconds();
			int		com_frameMsec = nowTime - lastTime;
			lastTime = nowTime;
			Printf( "frame:%i all:%3i gfr:%3i rf:%3i bk:%3i\n", com_frameNumber, com_frameMsec, time_gameFrame, time_frontend, time_backend );
			time_gameFrame = 0;
			time_gameDraw = 0;
		}

		com_frameNumber++;

		// set idLib frame number for frame based memory dumps
		idLib::frameNumber = com_frameNumber;
	}

	catch( idException & ) {
		return;			// an ERP_DROP was thrown
	}
}