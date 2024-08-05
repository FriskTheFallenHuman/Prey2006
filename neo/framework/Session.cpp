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

#include "Session_local.h"
#include "../ui/Window.h"
#include "../ui/UserInterfaceLocal.h"
#include "../sound/snd_local.h"

#define CDKEY_FILEPATH "../" BASE_GAMEDIR "/" CDKEY_FILE

idCVar	idSessionLocal::com_showAngles( "com_showAngles", "0", CVAR_SYSTEM | CVAR_BOOL, "" );
idCVar	idSessionLocal::com_minTics( "com_minTics", "1", CVAR_SYSTEM, "" );
idCVar	idSessionLocal::com_showTics( "com_showTics", "0", CVAR_SYSTEM | CVAR_BOOL, "" );
idCVar	idSessionLocal::com_showDemo( "com_showDemo", "0", CVAR_SYSTEM | CVAR_BOOL, "" );
idCVar	idSessionLocal::com_guid( "com_guid", "", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_ROM, "" );

idSessionLocal		sessLocal;
idSession			*session = &sessLocal;

// these must be kept up to date with window Levelshot in guis/mainmenu.gui
const int PREVIEW_X = 211;
const int PREVIEW_Y = 31;
const int PREVIEW_WIDTH = 398;
const int PREVIEW_HEIGHT = 298;

void RandomizeStack( void ) {
	// attempt to force uninitialized stack memory bugs
	int		bytes = 4000000;
	byte	*buf = (byte *)_alloca( bytes );

	int	fill = rand()&255;
	for ( int i = 0 ; i < bytes ; i++ ) {
		buf[i] = fill;
	}
}

/*
=================
Session_RescanSI_f
=================
*/
CONSOLE_COMMAND_SHIP( rescanSI, "internal - rescan serverinfo cvars and tell game", NULL ) {
	sessLocal.mapSpawnData.serverInfo = *cvarSystem->MoveCVarsToDict( CVAR_SERVERINFO );
	if ( game && idAsyncNetwork::server.IsActive() ) {
		game->SetServerInfo( sessLocal.mapSpawnData.serverInfo );
	}
}

/*
==================
Sess_WritePrecache_f
==================
*/
CONSOLE_COMMAND( writePrecache, "writes precache commands", NULL ) {
	if ( args.Argc() != 2 ) {
		common->Printf( "USAGE: writePrecache <execFile>\n" );
		return;
	}
	idStr	str = args.Argv(1);
	str.DefaultFileExtension( ".cfg" );
	idFile *f = fileSystem->OpenFileWrite( str, "fs_configpath" );
	declManager->WritePrecacheCommands( f );
	renderModelManager->WritePrecacheCommands( f );
	uiManager->WritePrecacheCommands( f );

	fileSystem->CloseFile( f );
}

/*
===============
idSessionLocal::MaybeWaitOnCDKey
===============
*/
bool idSessionLocal::MaybeWaitOnCDKey( void ) {
	if ( authEmitTimeout > 0 ) {
		authWaitBox = true;
		sessLocal.MessageBox( MSG_WAIT, common->GetLanguageDict()->GetString( "#str_07191" ), NULL, true, NULL, NULL, true );
		return true;
	}
	return false;
}

/*
===================
Session_PromptKey_f
===================
*/
CONSOLE_COMMAND_SHIP( promptKey, "prompt and sets the CD Key", NULL ) {
	const char	*retkey;
	bool		valid[ 2 ];
	static bool recursed = false;

	if ( recursed ) {
		common->Warning( "promptKey recursed - aborted" );
		return;
	}
	recursed = true;

	do {
		// in case we're already waiting for an auth to come back to us ( may happen exceptionally )
		if ( sessLocal.MaybeWaitOnCDKey() ) {
			if ( sessLocal.CDKeysAreValid( true ) ) {
				recursed = false;
				return;
			}
		}
		// the auth server may have replied and set an error message, otherwise use a default
		const char *prompt_msg = sessLocal.GetAuthMsg();
		if ( prompt_msg[ 0 ] == '\0' ) {
			prompt_msg = common->GetLanguageDict()->GetString( "#str_04308" );
		}
		retkey = sessLocal.MessageBox( MSG_CDKEY, prompt_msg, common->GetLanguageDict()->GetString( "#str_04305" ), true, NULL, NULL, true );
		if ( retkey ) {
			if ( sessLocal.CheckKey( retkey, false, valid ) ) {
				// if all went right, then we may have sent an auth request to the master ( unless the prompt is used during a net connect )
				bool canExit = true;
				if ( sessLocal.MaybeWaitOnCDKey() ) {
					// wait on auth reply, and got denied, prompt again
					if ( !sessLocal.CDKeysAreValid( true ) ) {
						// server says key is invalid - MaybeWaitOnCDKey was interrupted by a CDKeysAuthReply call, which has set the right error message
						// the invalid keys have also been cleared in the process
						sessLocal.MessageBox( MSG_OK, sessLocal.GetAuthMsg(), common->GetLanguageDict()->GetString( "#str_04310" ), true, NULL, NULL, true );
						canExit = false;
					}
				}
				if ( canExit ) {
					// make sure that's saved on file
					sessLocal.WriteCDKey();
					sessLocal.MessageBox( MSG_OK, common->GetLanguageDict()->GetString( "#str_04307" ), common->GetLanguageDict()->GetString( "#str_04305" ), true, NULL, NULL, true );
					break;
				}
			} else {
				// offline check sees key invalid
				// build a message about keys being wrong. do not attempt to change the current key state though
				// ( the keys may be valid, but user would have clicked on the dialog anyway, that kind of thing )
				idStr msg;
				idAsyncNetwork::BuildInvalidKeyMsg( msg, valid );
				sessLocal.MessageBox( MSG_OK, msg, common->GetLanguageDict()->GetString( "#str_04310" ), true, NULL, NULL, true );
			}
		} else if ( args.Argc() == 2 && idStr::Icmp( args.Argv(1), "force" ) == 0 ) {
			// cancelled in force mode
			cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "quit\n" );
			cmdSystem->ExecuteCommandBuffer();
		}
	} while ( retkey );
	recursed = false;
}

/*
===============================================================================

SESSION LOCAL

===============================================================================
*/

/*
===============
idSessionLocal::Clear
===============
*/
void idSessionLocal::Clear() {

	insideUpdateScreen = false;
	insideExecuteMapChange = false;

	loadingSaveGame = false;
	savegameFile = NULL;
	savegameVersion = 0;

	currentMapName.Clear();
	aviDemoShortName.Clear();
	msgFireBack[ 0 ].Clear();
	msgFireBack[ 1 ].Clear();

	timeHitch = 0;

	rw = NULL;
	sw = NULL;
	menuSoundWorld = NULL;
	readDemo = NULL;
	writeDemo = NULL;
	renderdemoVersion = 0;
	cmdDemoFile = NULL;

	syncNextGameFrame = false;
	mapSpawned = false;
	guiActive = NULL;
	aviCaptureMode = false;
	timeDemo = TD_NO;
	waitingOnBind = false;
	lastPacifierTime = 0;

	msgRunning = false;
	guiMsgRestore = NULL;
	msgIgnoreButtons = false;

	bytesNeededForMapLoad = 0;

#if ID_CONSOLE_LOCK
	emptyDrawCount = 0;
#endif
	ClearWipe();

	loadGameList.Clear();
	modsList.Clear();

	authEmitTimeout = 0;
	authWaitBox = false;

	authMsg.Clear();
}

/*
===============
idSessionLocal::idSessionLocal
===============
*/
idSessionLocal::idSessionLocal() {
	guiInGame = guiMainMenu = guiIntro \
		= guiRestartMenu = guiLoading = guiActive \
		= guiTest = guiMsg = guiMsgRestore = NULL;

	menuSoundWorld = NULL;

	demoversion=false;

	guiSubtitles = NULL;
	subtitleTextScaleInited = false;
	for ( int m = 0; m < sizeof( subtitlesTextScale ) / sizeof( subtitlesTextScale[0] ); m++ ) {
		subtitlesTextScale[m] = 0.0f;
	}

	Clear();
}

/*
===============
idSessionLocal::~idSessionLocal
===============
*/
idSessionLocal::~idSessionLocal() {
}

/*
===============
idSessionLocal::Stop

called on errors and game exits
===============
*/
void idSessionLocal::Stop() {
	ClearWipe();

	// clear mapSpawned and demo playing flags
	UnloadMap();

	// disconnect async client
	idAsyncNetwork::client.DisconnectFromServer();

	// kill async server
	idAsyncNetwork::server.Kill();

	if ( sw ) {
		sw->StopAllSounds();
	}

	insideUpdateScreen = false;
	insideExecuteMapChange = false;

	// drop all guis
	SetGUI( NULL, NULL );
}

/*
===============
idSessionLocal::Shutdown
===============
*/
void idSessionLocal::Shutdown() {
	int i;

	if ( aviCaptureMode ) {
		EndAVICapture();
	}

	if(timeDemo == TD_YES) {
		// else the game freezes when showing the timedemo results
		timeDemo = TD_YES_THEN_QUIT;
	}

	Stop();

	if ( rw ) {
		delete rw;
		rw = NULL;
	}

	if ( sw ) {
		delete sw;
		sw = NULL;
	}

	if ( menuSoundWorld ) {
		delete menuSoundWorld;
		menuSoundWorld = NULL;
	}

	mapSpawnData.serverInfo.Clear();
	mapSpawnData.syncedCVars.Clear();
	for ( i = 0; i < MAX_ASYNC_CLIENTS; i++ ) {
		mapSpawnData.userInfo[i].Clear();
		mapSpawnData.persistentPlayerInfo[i].Clear();
	}

	if ( guiMainMenu_MapList != NULL ) {
		guiMainMenu_MapList->Shutdown();
		uiManager->FreeListGUI( guiMainMenu_MapList );
		guiMainMenu_MapList = NULL;
	}

	Clear();
}

/*
===============
idSessionLocal::IsMultiplayer
===============
*/
bool	idSessionLocal::IsMultiplayer() {
	return idAsyncNetwork::IsActive();
}

/*
================
idSessionLocal::ShowLoadingGui
================
*/
void idSessionLocal::ShowLoadingGui() {
	if ( com_ticNumber == 0 ) {
		return;
	}
	console->Close();

	// introduced in D3XP code. don't think it actually fixes anything, but doesn't hurt either
#if 1
	// Try and prevent the while loop from being skipped over (long hitch on the main thread?)
	int stop = Sys_Milliseconds() + 1000;
	int force = 10;
	while ( Sys_Milliseconds() < stop || force-- > 0 ) {
		com_frameTime = com_ticNumber * USERCMD_MSEC;
		session->Frame();
		session->UpdateScreen( false );
	}
#else
	int stop = com_ticNumber + 1000.0f / USERCMD_MSEC * 1.0f;
	while ( com_ticNumber < stop ) {
		com_frameTime = com_ticNumber * USERCMD_MSEC;
		session->Frame();
		session->UpdateScreen( false );
	}
#endif
}

/*
================
Session_TestGUI_f
================
*/
CONSOLE_COMMAND_SHIP( testGUI, "tests a gui", NULL ) {
	sessLocal.TestGUI( args.Argv(1) );
}

/*
================
idSessionLocal::TestGUI
================
*/
void idSessionLocal::TestGUI( const char *guiName ) {
	if ( guiName && *guiName ) {
		guiTest = uiManager->FindGui( guiName, true, false, true );
	} else {
		guiTest = NULL;
	}
}

/*
================
Session_Disconnect_f
================
*/
CONSOLE_COMMAND_SHIP( disconnect, "disconnects from a game", NULL ) {
	sessLocal.Stop();
	sessLocal.StartMenu();
	if ( soundSystem ) {
		soundSystem->SetMute( false );
	}
}

/*
===============
Session_Hitch_f
===============
*/
CONSOLE_COMMAND( hitch, "hitches the game", NULL ) {
	idSoundWorld *sw = soundSystem->GetPlayingSoundWorld();
	if ( sw ) {
		soundSystem->SetMute(true);
		sw->Pause();
		Sys_EnterCriticalSection();
	}
	if ( args.Argc() == 2 ) {
		Sys_Sleep( atoi(args.Argv(1)) );
	} else {
		Sys_Sleep( 100 );
	}
	if ( sw ) {
		Sys_LeaveCriticalSection();
		sw->UnPause();
		soundSystem->SetMute(false);
	}
}

/*
===============
idSessionLocal::ProcessEvent
===============
*/
bool idSessionLocal::ProcessEvent( const sysEvent_t *event ) {
	// hitting escape anywhere brings up the menu
	// DG: but shift-escape should bring up console instead so ignore that
	if ( !guiActive && event->evType == SE_KEY && event->evValue2 == 1
			&& event->evValue == K_ESCAPE && !idKeyInput::IsDown( K_SHIFT ) ) {
		console->Close();
		if ( game ) {
			idUserInterface	*gui = NULL;
			escReply_t		op;
			op = game->HandleESC( &gui );
			if ( op == ESC_IGNORE ) {
				return true;
			} else if ( op == ESC_GUI ) {
				SetGUI( gui, NULL );
				return true;
			}
		}
		StartMenu();
		return true;
	}

	// let the pull-down console take it if desired
	if ( console->ProcessEvent( event, false ) ) {
		return true;
	}

	// if we are testing a GUI, send all events to it
	if ( guiTest ) {
		// hitting escape exits the testgui
		if ( event->evType == SE_KEY && event->evValue2 == 1 && event->evValue == K_ESCAPE ) {
			guiTest = NULL;
			return true;
		}

		static const char *cmd;
		cmd = guiTest->HandleEvent( event, com_frameTime );
		if ( cmd && cmd[0] ) {
			common->Printf( "testGui event returned: '%s'\n", cmd );
		}
		return true;
	}

	// menus / etc
	if ( guiActive ) {
		MenuEvent( event );
		return true;
	}

	// if we aren't in a game, force the console to take it
	if ( !mapSpawned ) {
		console->ProcessEvent( event, true );
		return true;
	}

	// in game, exec bindings for all key downs
	if ( event->evType == SE_KEY && event->evValue2 == 1 ) {
		idKeyInput::ExecKeyBinding( event->evValue );
		return true;
	}

	return false;
}

/*
===============
idSessionLocal::PacifierUpdate
===============
*/
void idSessionLocal::PacifierUpdate() {
	if ( !insideExecuteMapChange ) {
		return;
	}

	// never do pacifier screen updates while inside the
	// drawing code, or we can have various recursive problems
	if ( insideUpdateScreen ) {
		return;
	}

	int	time = eventLoop->Milliseconds();

	if ( time - lastPacifierTime < 100 ) {
		return;
	}
	lastPacifierTime = time;

	if ( guiLoading && bytesNeededForMapLoad ) {
		float n = fileSystem->GetReadCount();
		float pct = ( n / bytesNeededForMapLoad );
		// pct = idMath::ClampFloat( 0.0f, 100.0f, pct );
		guiLoading->SetStateFloat( "map_loading", pct );
		guiLoading->StateChanged( com_frameTime );
	}

	Sys_GenerateEvents();

	UpdateScreen();

	idAsyncNetwork::client.PacifierUpdate();
	idAsyncNetwork::server.PacifierUpdate();
}

/*
===============
idSessionLocal::Frame
===============
*/
extern bool CheckOpenALDeviceAndRecoverIfNeeded();
extern int g_screenshotFormat;
void idSessionLocal::Frame() {

	if ( com_asyncSound.GetInteger() == 0 ) {
		soundSystem->AsyncUpdateWrite( Sys_Milliseconds() );
	}

	// DG: periodically check if sound device is still there and try to reset it if not
	//     (calling this from idSoundSystem::AsyncUpdate(), which runs in a separate thread
	//      by default, causes a deadlock when calling idCommon->Warning())
	CheckOpenALDeviceAndRecoverIfNeeded();

	// Editors that completely take over the game
	if ( com_editorActive && ( com_editors & ( EDITOR_RADIANT | EDITOR_GUI ) ) ) {
		return;
	}

#if 0 // handled via Sys_GenerateEvents() -> handleMouseGrab()
	// if the console is down, we don't need to hold
	// the mouse cursor
	if ( console->Active() || com_editorActive ) {
		Sys_GrabMouseCursor( false );
	} else {
		Sys_GrabMouseCursor( true );
	}
#endif

	// save the screenshot and audio from the last draw if needed
	if ( aviCaptureMode ) {
		idStr	name;

		name = va("demos/%s/%s_%05i.tga", aviDemoShortName.c_str(), aviDemoShortName.c_str(), aviTicStart );

		float ratio = 30.0f / ( 1000.0f / USERCMD_MSEC / com_aviDemoTics.GetInteger() );
		aviDemoFrameCount += ratio;
		if ( aviTicStart + 1 != ( int )aviDemoFrameCount ) {
			// skipped frames so write them out
			int c = aviDemoFrameCount - aviTicStart;
			while ( c-- ) {
				g_screenshotFormat = 0;
				renderSystem->TakeScreenshot( com_aviDemoWidth.GetInteger(), com_aviDemoHeight.GetInteger(), name, com_aviDemoSamples.GetInteger(), NULL );
				name = va("demos/%s/%s_%05i.tga", aviDemoShortName.c_str(), aviDemoShortName.c_str(), ++aviTicStart );
			}
		}
		aviTicStart = aviDemoFrameCount;

		// remove any printed lines at the top before taking the screenshot
		console->ClearNotifyLines();

		// this will call Draw, possibly multiple times if com_aviDemoSamples is > 1
		g_screenshotFormat = 0;
		renderSystem->TakeScreenshot( com_aviDemoWidth.GetInteger(), com_aviDemoHeight.GetInteger(), name, com_aviDemoSamples.GetInteger(), NULL );
	}

	// at startup, we may be backwards
	if ( latchedTicNumber > com_ticNumber ) {
		latchedTicNumber = com_ticNumber;
	}

	// se how many tics we should have before continuing
	int	minTic = latchedTicNumber + 1;
	if ( com_minTics.GetInteger() > 1 ) {
		minTic = lastGameTic + com_minTics.GetInteger();
	}

	if ( readDemo ) {
		if ( !timeDemo && numDemoFrames != 1 ) {
			minTic = lastDemoTic + USERCMD_PER_DEMO_FRAME;
		} else {
			// timedemos and demoshots will run as fast as they can, other demos
			// will not run more than 30 hz
			minTic = latchedTicNumber;
		}
	} else if ( writeDemo ) {
		minTic = lastGameTic + USERCMD_PER_DEMO_FRAME;		// demos are recorded at 30 hz
	}

	// fixedTic lets us run a forced number of usercmd each frame without timing
	if ( com_fixedTic.GetInteger() ) {
		minTic = latchedTicNumber;
	}

	while( 1 ) {
		latchedTicNumber = com_ticNumber;
		if ( latchedTicNumber >= minTic ) {
			break;
		}
		Sys_WaitForEvent( TRIGGER_EVENT_ONE );
	}

	if ( authEmitTimeout ) {
		// waiting for a game auth
		if ( Sys_Milliseconds() > authEmitTimeout ) {
			// expired with no reply
			// means that if a firewall is blocking the master, we will let through
			common->DPrintf( "no reply from auth\n" );
			if ( authWaitBox ) {
				// close the wait box
				StopBox();
				authWaitBox = false;
			}
			if ( cdkey_state == CDKEY_CHECKING ) {
				cdkey_state = CDKEY_OK;
			}
			// maintain this empty as it's set by auth denials
			authMsg.Empty();
			authEmitTimeout = 0;
			SetCDKeyGuiVars();
		}
	}

	// send frame and mouse events to active guis
	GuiFrameEvents();

	// advance demos
	if ( readDemo ) {
		AdvanceRenderDemo( false );
		return;
	}

	//------------ single player game tics --------------

	if ( !mapSpawned || guiActive ) {
		if ( !com_asyncInput.GetBool() ) {
			// early exit, won't do RunGameTic .. but still need to update mouse position for GUIs
			usercmdGen->GetDirectUsercmd();
		}
	}

	if ( !mapSpawned ) {
		return;
	}

	if ( guiActive ) {
		lastGameTic = latchedTicNumber;
		return;
	}

	// in message box / GUIFrame, idSessionLocal::Frame is used for GUI interactivity
	// but we early exit to avoid running game frames
	if ( idAsyncNetwork::IsActive() ) {
		return;
	}

	// check for user info changes
	if ( cvarSystem->GetModifiedFlags() & CVAR_USERINFO ) {
		mapSpawnData.userInfo[0] = *cvarSystem->MoveCVarsToDict( CVAR_USERINFO );
		game->SetUserInfo( 0, mapSpawnData.userInfo[0], false, false );
		cvarSystem->ClearModifiedFlags( CVAR_USERINFO );
	}

	// see how many usercmds we are going to run
	int	numCmdsToRun = latchedTicNumber - lastGameTic;

	// don't let a long onDemand sound load unsync everything
	if ( timeHitch ) {
		int	skip = timeHitch / USERCMD_MSEC;
		lastGameTic += skip;
		numCmdsToRun -= skip;
		timeHitch = 0;
	}

	// don't get too far behind after a hitch
	if ( numCmdsToRun > 10 ) {
		lastGameTic = latchedTicNumber - 10;
	}

	// never use more than USERCMD_PER_DEMO_FRAME,
	// which makes it go into slow motion when recording
	if ( writeDemo ) {
		int fixedTic = USERCMD_PER_DEMO_FRAME;
		// we should have waited long enough
		if ( numCmdsToRun < fixedTic ) {
			common->Error( "idSessionLocal::Frame: numCmdsToRun < fixedTic" );
		}
		// we may need to dump older commands
		lastGameTic = latchedTicNumber - fixedTic;
	} else if ( com_fixedTic.GetInteger() > 0 ) {
		// this may cause commands run in a previous frame to
		// be run again if we are going at above the real time rate
		lastGameTic = latchedTicNumber - com_fixedTic.GetInteger();
	} else if (	aviCaptureMode ) {
		lastGameTic = latchedTicNumber - com_aviDemoTics.GetInteger();
	}

	// force only one game frame update this frame.  the game code requests this after skipping cinematics
	// so we come back immediately after the cinematic is done instead of a few frames later which can
	// cause sounds played right after the cinematic to not play.
	if ( syncNextGameFrame ) {
		lastGameTic = latchedTicNumber - 1;
		syncNextGameFrame = false;
	}

	// create client commands, which will be sent directly
	// to the game
	if ( com_showTics.GetBool() ) {
		common->Printf( "%i ", latchedTicNumber - lastGameTic );
	}

	int	gameTicsToRun = latchedTicNumber - lastGameTic;
	int i;

	soundSystemLocal.SF_ShowSubtitle();

	for ( i = 0 ; i < gameTicsToRun ; i++ ) {
		RunGameTic();
		if ( !mapSpawned ) {
			// exited game play
			break;
		}
		if ( syncNextGameFrame ) {
			// long game frame, so break out and continue executing as if there was no hitch
			break;
		}
	}
}

/*
================
idSessionLocal::RunGameTic
================
*/
void idSessionLocal::RunGameTic() {
	logCmd_t	logCmd;
	usercmd_t	cmd;

	// if we are doing a command demo, read or write from the file
	if ( cmdDemoFile ) {
		if ( !cmdDemoFile->Read( &logCmd, sizeof( logCmd ) ) ) {
			common->Printf( "Command demo completed at logIndex %i\n", logIndex );
			fileSystem->CloseFile( cmdDemoFile );
			cmdDemoFile = NULL;
			if ( aviCaptureMode ) {
				EndAVICapture();
				Shutdown();
			}
			// we fall out of the demo to normal commands
			// the impulse and chat character toggles may not be correct, and the view
			// angle will definitely be wrong
		} else {
			cmd = logCmd.cmd;
			cmd.ByteSwap();
			logCmd.consistencyHash = LittleInt( logCmd.consistencyHash );
		}
	}

	// if we didn't get one from the file, get it locally
	if ( !cmdDemoFile ) {
		// get a locally created command
		if ( com_asyncInput.GetBool() ) {
			cmd = usercmdGen->TicCmd( lastGameTic );
		} else {
			cmd = usercmdGen->GetDirectUsercmd();
		}
		lastGameTic++;
	}

	// run the game logic every player move
	int	start = Sys_Milliseconds();
	gameReturn_t	ret = game->RunFrame( &cmd );

	int end = Sys_Milliseconds();
	time_gameFrame += end - start;	// note time used for com_speeds

	// check for constency failure from a recorded command
	if ( cmdDemoFile ) {
		if ( ret.consistencyHash != logCmd.consistencyHash ) {
			common->Printf( "Consistency failure on logIndex %i\n", logIndex );
			Stop();
			return;
		}
	}

	// save the cmd for cmdDemo archiving
	if ( logIndex < MAX_LOGGED_USERCMDS ) {
		loggedUsercmds[logIndex].cmd = cmd;
		// save the consistencyHash for demo playback verification
		loggedUsercmds[logIndex].consistencyHash = ret.consistencyHash;
		if (logIndex % 30 == 0 && statIndex < MAX_LOGGED_STATS) {
			loggedStats[statIndex].health = ret.health;
			loggedStats[statIndex].heartRate = ret.heartRate;
			loggedStats[statIndex].stamina = ret.stamina;
			loggedStats[statIndex].combat = ret.combat;
			statIndex++;
		}
		logIndex++;
	}

	syncNextGameFrame = ret.syncNextGameFrame;

	if ( ret.sessionCommand[0] ) {
		idCmdArgs args;

		args.TokenizeString( ret.sessionCommand, false );

		if ( !idStr::Icmp( args.Argv(0), "map" ) ) {
			// get current player states
			for ( int i = 0 ; i < numClients ; i++ ) {
				mapSpawnData.persistentPlayerInfo[i] = game->GetPersistentPlayerInfo( i );
			}
			// clear the devmap key on serverinfo, so player spawns
			// won't get the map testing items
			mapSpawnData.serverInfo.Delete( "devmap" );

			const char* deathwalkmap = GetDeathwalkMapName( args.Argv( 1 ) );
			mapSpawnData.serverInfo.Set( "deathwalkmap", deathwalkmap );
			mapSpawnData.serverInfo.SetBool( "shouldappendlevel", deathwalkmap && deathwalkmap[0] );

			// go to the next map
			MoveToNewMap( args.Argv(1) );
		} else if ( !idStr::Icmp( args.Argv(0), "devmap" ) ) {
			mapSpawnData.serverInfo.Set( "devmap", "1" );

			const char *deathwalkmap = GetDeathwalkMapName( args.Argv( 1 ) );
			mapSpawnData.serverInfo.Set( "deathwalkmap", deathwalkmap );
			mapSpawnData.serverInfo.SetBool( "shouldappendlevel", deathwalkmap && deathwalkmap[0] );

			MoveToNewMap( args.Argv(1) );
		} else if ( !idStr::Icmp( args.Argv(0), "died" ) ) {
			// restart on the same map
			UnloadMap();
			SetGUI(guiRestartMenu, NULL);
		} else if ( !idStr::Icmp( args.Argv(0), "disconnect" ) ) {
			cmdSystem->BufferCommandText( CMD_EXEC_INSERT, "stoprecording ; disconnect" );
		}
	}
}

/*
===============
idSessionLocal::Init

Called in an orderly fashion at system startup,
so commands, cvars, files, etc are all available
===============
*/
void idSessionLocal::Init() {

	common->Printf( "----- Initializing Session -----\n" );

	// the same idRenderWorld will be used for all games
	// and demos, insuring that level specific models
	// will be freed
	rw = renderSystem->AllocRenderWorld();
	sw = soundSystem->AllocSoundWorld( rw );

	menuSoundWorld = soundSystem->AllocSoundWorld( rw );

	// we have a single instance of the main menu
	guiMainMenu = uiManager->FindGui( "guis/mainmenu.gui", true, false, true );
	if (!guiMainMenu) {
		guiMainMenu = uiManager->FindGui( "guis/demo_mainmenu.gui", true, false, true );
		demoversion = (guiMainMenu != NULL);
	}
	guiMainMenu_MapList = uiManager->AllocListGUI();
	guiMainMenu_MapList->Config( guiMainMenu, "mapList" );
	idAsyncNetwork::client.serverList.GUIConfig( guiMainMenu, "serverList" );
	guiRestartMenu = uiManager->FindGui( "guis/restart.gui", true, false, true );
	guiSubtitles = uiManager->FindGui( "guis/subtitles.gui", true, false, true );
	if ( guiSubtitles ) {
		idWindow *desktop = ( (idUserInterfaceLocal *)guiSubtitles )->GetDesktop();
		if ( desktop ) {
#define SUBTITLE_GET_TEXT_SCALE(name, index) \
			{                              \
				drawWin_t *dw = desktop->FindChildByName( name ); \
				if ( dw && dw->win ) \
				{ \
					idWinVar *winvar = dw->win->GetWinVarByName( "textScale" ); \
					if ( winvar ) \
						subtitlesTextScale[index] = winvar->x(); \
				} \
			}
			SUBTITLE_GET_TEXT_SCALE( "subtitles1", 0 )
			SUBTITLE_GET_TEXT_SCALE( "subtitles2", 1 )
			SUBTITLE_GET_TEXT_SCALE( "subtitles3", 2 )
#undef SUBTITLE_GET_TEXT_SCALE
		}
	}
	guiMsg = uiManager->FindGui( "guis/msg.gui", true, false, true );
	guiIntro = uiManager->FindGui( "guis/intro.gui", true, false, true );

	whiteMaterial = declManager->FindMaterial( "_white" );

	guiInGame = NULL;
	guiTest = NULL;

	guiActive = NULL;
	guiHandle = NULL;

	ReadCDKey();
}

/*
===============
idSessionLocal::GetLocalClientNum
===============
*/
int idSessionLocal::GetLocalClientNum() {
	if ( idAsyncNetwork::client.IsActive() ) {
		return idAsyncNetwork::client.GetLocalClientNum();
	} else if ( idAsyncNetwork::server.IsActive() ) {
		if ( idAsyncNetwork::serverDedicated.GetInteger() == 0 ) {
			return 0;
		} else if ( idAsyncNetwork::server.IsClientInGame( idAsyncNetwork::serverDrawClient.GetInteger() ) ) {
			return idAsyncNetwork::serverDrawClient.GetInteger();
		} else {
			return -1;
		}
	} else {
		return 0;
	}
}

/*
===============
idSessionLocal::SetPlayingSoundWorld
===============
*/
void idSessionLocal::SetPlayingSoundWorld() {
	if ( guiActive && ( guiActive == guiMainMenu || guiActive == guiIntro || guiActive == guiLoading || ( guiActive == guiMsg && !mapSpawned ) ) ) {
		soundSystem->SetPlayingSoundWorld( menuSoundWorld );
	} else {
		soundSystem->SetPlayingSoundWorld( sw );
	}
}

/*
===============
idSessionLocal::TimeHitch

this is used by the sound system when an OnDemand sound is loaded, so the game action
doesn't advance and get things out of sync
===============
*/
void idSessionLocal::TimeHitch( int msec ) {
	timeHitch += msec;
}

/*
=================
idSessionLocal::ReadCDKey
=================
*/
void idSessionLocal::ReadCDKey( void ) {
	idStr filename;
	idFile *f;
	char buffer[32];

	cdkey_state = CDKEY_UNKNOWN;

	filename = CDKEY_FILEPATH;
	f = fileSystem->OpenExplicitFileRead( fileSystem->RelativePathToOSPath( filename, "fs_configpath" ) );

	// try the install path, which is where the cd installer and steam put it
	if ( !f )
		f = fileSystem->OpenExplicitFileRead( fileSystem->RelativePathToOSPath( filename, "fs_basepath" ) );

	if ( !f ) {
		common->Printf( "Couldn't read %s.\n", filename.c_str() );
		cdkey[ 0 ] = '\0';
	} else {
		memset( buffer, 0, sizeof(buffer) );
		f->Read( buffer, CDKEY_BUF_LEN - 1 );
		fileSystem->CloseFile( f );
		idStr::Copynz( cdkey, buffer, CDKEY_BUF_LEN );
	}
}

/*
================
idSessionLocal::WriteCDKey
================
*/
void idSessionLocal::WriteCDKey( void ) {
	idStr filename;
	idFile *f;
	const char *OSPath;

	filename = CDKEY_FILEPATH;
	// OpenFileWrite advertises creating directories to the path if needed, but that won't work with a '..' in the path
	// occasionally on windows, but mostly on Linux and OSX, the fs_configpath/base may not exist in full
	OSPath = fileSystem->BuildOSPath( cvarSystem->GetCVarString( "fs_configpath" ), BASE_GAMEDIR, CDKEY_FILE );
	fileSystem->CreateOSPath( OSPath );
	f = fileSystem->OpenFileWrite( filename, "fs_configpath" );
	if ( !f ) {
		common->Printf( "Couldn't write %s.\n", filename.c_str() );
		return;
	}
	f->Printf( "%s%s", cdkey, CDKEY_TEXT );
	fileSystem->CloseFile( f );
}

/*
===============
idSessionLocal::ClearKey
===============
*/
void idSessionLocal::ClearCDKey( bool valid[ 2 ] ) {
	if ( !valid[ 0 ] ) {
		memset( cdkey, 0, CDKEY_BUF_LEN );
		cdkey_state = CDKEY_UNKNOWN;
	} else if ( cdkey_state == CDKEY_CHECKING ) {
		// if a key was in checking and not explicitely asked for clearing, put it back to ok
		cdkey_state = CDKEY_OK;
	}

	WriteCDKey( );
}

/*
================
idSessionLocal::GetCDKey
================
*/
const char *idSessionLocal::GetCDKey( bool xp ) {
	if ( cdkey_state == CDKEY_OK || cdkey_state == CDKEY_CHECKING ) {
		return cdkey;
	}
	return NULL;
}

// digits to letters table
#define CDKEY_DIGITS "TWSBJCGD7PA23RLH"

/*
===============
idSessionLocal::EmitGameAuth
we toggled some key state to CDKEY_CHECKING. send a standalone auth packet to validate
===============
*/
void idSessionLocal::EmitGameAuth( void ) {
	// make sure the auth reply is empty, we use it to indicate an auth reply
	authMsg.Empty();
	if ( idAsyncNetwork::client.SendAuthCheck( cdkey_state == CDKEY_CHECKING ? cdkey : NULL, NULL ) ) {
		authEmitTimeout = Sys_Milliseconds() + CDKEY_AUTH_TIMEOUT;
		common->DPrintf( "authing with the master..\n" );
	} else {
		// net is not available
		common->DPrintf( "sendAuthCheck failed\n" );
		if ( cdkey_state == CDKEY_CHECKING ) {
			cdkey_state = CDKEY_OK;
		}
	}
}

/*
================
idSessionLocal::CheckKey
the function will only modify keys to _OK or _CHECKING if the offline checks are passed
if the function returns false, the offline checks failed, and offline_valid holds which keys are bad
================
*/
bool idSessionLocal::CheckKey( const char *key, bool netConnect, bool offline_valid[ 2 ] ) {
	char lkey[ 2 ][ CDKEY_BUF_LEN ];
	char l_chk[ 2 ][ 3 ];
	char s_chk[ 3 ];
	int i_key;
	unsigned int checksum, chk8;
	bool edited_key[ 2 ];

	// make sure have a right input string
	assert( strlen( key ) == ( CDKEY_BUF_LEN - 1 ) * 2 + 4 + 3 + 4 );

	edited_key[ 0 ] = ( key[0] == '1' );
	idStr::Copynz( lkey[0], key + 2, CDKEY_BUF_LEN );
	idStr::ToUpper( lkey[0] );
	idStr::Copynz( l_chk[0], key + CDKEY_BUF_LEN + 2, 3 );
	idStr::ToUpper( l_chk[0] );

	offline_valid[ 0 ] = offline_valid[ 1 ] = true;
	for( i_key = 0; i_key < 1; i_key++ ) {
		// check that the characters are from the valid set
		int i;
		for ( i = 0; i < CDKEY_BUF_LEN - 1; i++ ) {
			if ( !strchr( CDKEY_DIGITS, lkey[i_key][i] ) ) {
				offline_valid[ i_key ] = false;
				continue;
			}
		}

		if ( edited_key[ i_key ] ) {
			// verify the checksum for edited keys only
			checksum = CRC32_BlockChecksum( lkey[i_key], CDKEY_BUF_LEN - 1 );
			chk8 = ( checksum & 0xff ) ^ ( ( ( checksum & 0xff00 ) >> 8 ) ^ ( ( ( checksum & 0xff0000 ) >> 16 ) ^ ( ( checksum & 0xff000000 ) >> 24 ) ) );
			idStr::snPrintf( s_chk, 3, "%02X", chk8 );
			if ( idStr::Icmp( l_chk[i_key], s_chk ) != 0 ) {
				offline_valid[ i_key ] = false;
				continue;
			}
		}
	}

	if ( !offline_valid[ 0 ] || !offline_valid[1] ) {
		return false;
	}

	// offline checks passed, we'll return true and optionally emit key check requests
	// the function should only modify the key states if the offline checks passed successfully

	// set the keys, don't send a game auth if we are net connecting
	idStr::Copynz( cdkey, lkey[0], CDKEY_BUF_LEN );
	netConnect ? cdkey_state = CDKEY_OK : cdkey_state = CDKEY_CHECKING;
	if ( !netConnect ) {
		EmitGameAuth();
	}
	SetCDKeyGuiVars();

	return true;
}

/*
===============
idSessionLocal::CDKeysAreValid
checking that the key is present and uses only valid characters
if d3xp is installed, check for a valid xpkey as well
emit an auth packet to the master if possible and needed
===============
*/
bool idSessionLocal::CDKeysAreValid( bool strict ) {
	int i;
	bool emitAuth = false;

	if ( cdkey_state == CDKEY_UNKNOWN ) {
		if ( strlen( cdkey ) != CDKEY_BUF_LEN - 1 ) {
			cdkey_state = CDKEY_INVALID;
		} else {
			for ( i = 0; i < CDKEY_BUF_LEN-1; i++ ) {
				if ( !strchr( CDKEY_DIGITS, cdkey[i] ) ) {
					cdkey_state = CDKEY_INVALID;
					break;
				}
			}
		}
		if ( cdkey_state == CDKEY_UNKNOWN ) {
			cdkey_state = CDKEY_CHECKING;
			emitAuth = true;
		}
	}

	if ( emitAuth ) {
		EmitGameAuth();
	}
	// make sure to keep the mainmenu gui up to date in case we made state changes
	SetCDKeyGuiVars();
	if ( strict ) {
		return cdkey_state == CDKEY_OK;
	} else {
		return ( cdkey_state == CDKEY_OK || cdkey_state == CDKEY_CHECKING );
	}
}

/*
===============
idSessionLocal::WaitingForGameAuth
===============
*/
bool idSessionLocal::WaitingForGameAuth( void ) {
	return authEmitTimeout != 0;
}

/*
===============
idSessionLocal::CDKeysAuthReply
===============
*/
void idSessionLocal::CDKeysAuthReply( bool valid, const char *auth_msg ) {
	//assert( authEmitTimeout > 0 );
	if ( authWaitBox ) {
		// close the wait box
		StopBox();
		authWaitBox = false;
	}
	if ( !valid ) {
		common->DPrintf( "auth key is invalid\n" );
		authMsg = auth_msg;
		if ( cdkey_state == CDKEY_CHECKING ) {
			cdkey_state = CDKEY_INVALID;
		}
	} else {
		common->DPrintf( "client is authed in\n" );
		if ( cdkey_state == CDKEY_CHECKING ) {
			cdkey_state = CDKEY_OK;
		}
	}
	authEmitTimeout = 0;
	SetCDKeyGuiVars();
}

/*
===============
idSessionLocal::GetCurrentMapName
===============
*/
const char *idSessionLocal::GetCurrentMapName() {
	return currentMapName.c_str();
}

/*
===============
idSessionLocal::GetSaveGameVersion
===============
*/
int idSessionLocal::GetSaveGameVersion( void ) {
	return savegameVersion;
}

/*
===============
idSessionLocal::GetAuthMsg
===============
*/
const char *idSessionLocal::GetAuthMsg( void ) {
	return authMsg.c_str();
}

/*
===============
idSessionLocal::ShouldAppendLevel
===============
*/
bool idSessionLocal::ShouldAppendLevel( void ) const {
	return mapSpawnData.serverInfo.GetBool( "shouldappendlevel" );
}

/*
===============
idSessionLocal::GetDeathwalkMapName
===============
*/
const char * idSessionLocal::GetDeathwalkMapName( void ) const {
	idStr mapName = mapSpawnData.serverInfo.GetString( "si_map" );
	return GetDeathwalkMapName( mapName );
}

/*
===============
idSessionLocal::GetDeathwalkMapName
===============
*/
const char * idSessionLocal::GetDeathwalkMapName( const char *mapName ) const {
	const idDecl *mapDecl = declManager->FindType( DECL_MAPDEF, mapName, false );
#if 0
	if ( !mapDecl ) {
		mapDecl = declManager->FindType( DECL_MAPDEF, "defaultMap", false );
	}
#else
	if ( !mapDecl ) {
		return "";
	}
#endif
	const idDeclEntityDef *mapDef = static_cast<const idDeclEntityDef *>( mapDecl );
	if ( !mapDef ) {
		return "";
	}
	const char *dwMap = mapDef->dict.GetString( "deathwalkmap" );
	if ( !dwMap || !dwMap[0] ) {
		return "";
	}
	if ( !idStr::Icmp( dwMap, "none" ) ) {
		return "";
	}

	return dwMap;
}

/*
===============
idSessionLocal::ShowSubtitle
===============
*/
void idSessionLocal::ShowSubtitle( const idStrList &strList ) {
	int num;
	int i;
	int index;
	char text[32];

	if ( !guiSubtitles ) {
		return;
	}

	// setup subtitles's text scale
	if(!subtitleTextScaleInited )
	{
		idWindow *desktop = ( (idUserInterfaceLocal *)guiSubtitles )->GetDesktop();
		if( desktop ) {
#define SUBTITLE_SET_TEXT_SCALE(name, index) \
			{                              \
				float f = subtitlesTextScale[index]; \
				if( f > 0.0f ) \
				{ \
					sprintf( text, /*sizeof(text), */"%f", f ); \
					desktop->SetChildWinVarVal( name, "textScale", text ); \
				} \
			}
			SUBTITLE_SET_TEXT_SCALE( "subtitles1", 0 )
			SUBTITLE_SET_TEXT_SCALE( "subtitles2", 1 )
			SUBTITLE_SET_TEXT_SCALE( "subtitles3", 2 )
#undef SUBTITLE_SET_TEXT_SCALE
		}
		subtitleTextScaleInited = true;
	}

	num = strList.Num();
	for ( i = 0; i < 3; i++ ) {
		index = num - 1 - i;
		if ( index >= 0 ) {
			sprintf( text, /*sizeof(text), */"subtitleText%d", 3 - i );
			guiSubtitles->SetStateString( text, strList[index].c_str() );
			sprintf( text, /*sizeof(text), */"subtitleAlpha%d", 3 - i );
			guiSubtitles->SetStateFloat( text, 1 );
		} else {
			sprintf( text, /*sizeof(text), */"subtitleAlpha%d", 3 - i );
			guiSubtitles->SetStateFloat(text, 0);
		}
	}
	guiSubtitles->StateChanged( game->GetTimeGroupTime( 1 ) );
}

/*
===============
idSessionLocal::HideSubtitle
===============
*/
void idSessionLocal::HideSubtitle( void ) const {
	if ( !guiSubtitles ) {
		return;
	}

	guiSubtitles->SetStateFloat( "subtitleAlpha1", 0 );
	guiSubtitles->SetStateFloat( "subtitleAlpha2", 0 );
	guiSubtitles->SetStateFloat( "subtitleAlpha3", 0 );
	/*guiSubtitles->SetStateFloat( "subtitleAlpha4", 0 );
	guiSubtitles->SetStateFloat( "subtitleAlpha5", 0 );*/
}