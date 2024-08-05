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

/*
================
FindUnusedFileName
================
*/
static idStr FindUnusedFileName( const char *format ) {
	int i;
	char	filename[1024];

	for ( i = 0 ; i < 999 ; i++ ) {
		sprintf( filename, format, i );
		int len = fileSystem->ReadFile( filename, NULL, NULL );
		if ( len <= 0 ) {
			return filename;	// file doesn't exist
		}
	}

	return filename;
}

/*
================
idSessionLocal::StartRecordingRenderDemo
================
*/
void idSessionLocal::StartRecordingRenderDemo( const char *demoName ) {
	if ( writeDemo ) {
		// allow it to act like a toggle
		StopRecordingRenderDemo();
		return;
	}

	if ( !demoName[0] ) {
		common->Printf( "idSessionLocal::StartRecordingRenderDemo: no name specified\n" );
		return;
	}

	console->Close();

	writeDemo = new idDemoFile;
	if ( !writeDemo->OpenForWriting( demoName ) ) {
		common->Printf( "error opening %s\n", demoName );
		delete writeDemo;
		writeDemo = NULL;
		return;
	}

	common->Printf( "recording to %s\n", writeDemo->GetName() );

	writeDemo->WriteInt( DS_VERSION );
	writeDemo->WriteInt( RENDERDEMO_VERSION );

	// if we are in a map already, dump the current state
	sw->StartWritingDemo( writeDemo );
	rw->StartWritingDemo( writeDemo );
}

/*
================
idSessionLocal::StopRecordingRenderDemo
================
*/
void idSessionLocal::StopRecordingRenderDemo() {
	if ( !writeDemo ) {
		common->Printf( "idSessionLocal::StopRecordingRenderDemo: not recording\n" );
		return;
	}
	sw->StopWritingDemo();
	rw->StopWritingDemo();

	writeDemo->Close();
	common->Printf( "stopped recording %s.\n", writeDemo->GetName() );
	delete writeDemo;
	writeDemo = NULL;
}

/*
================
idSessionLocal::StopPlayingRenderDemo

Reports timeDemo numbers and finishes any avi recording
================
*/
void idSessionLocal::StopPlayingRenderDemo() {
	if ( !readDemo ) {
		timeDemo = TD_NO;
		return;
	}

	// Record the stop time before doing anything that could be time consuming
	int timeDemoStopTime = Sys_Milliseconds();

	EndAVICapture();

	readDemo->Close();

	sw->StopAllSounds();
	soundSystem->SetPlayingSoundWorld( menuSoundWorld );

	common->Printf( "stopped playing %s.\n", readDemo->GetName() );
	delete readDemo;
	readDemo = NULL;

	if ( timeDemo ) {
		// report the stats
		float	demoSeconds = ( timeDemoStopTime - timeDemoStartTime ) * 0.001f;
		float	demoFPS = numDemoFrames / demoSeconds;
		idStr	message = va( "%i frames rendered in %3.1f seconds = %3.1f fps\n", numDemoFrames, demoSeconds, demoFPS );

		common->Printf( "%s", message.c_str() );
		if ( timeDemo == TD_YES_THEN_QUIT ) {
			cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "quit\n" );
		} else {
			soundSystem->SetMute( true );
			MessageBox( MSG_OK, message, "Time Demo Results", true );
			soundSystem->SetMute( false );
		}
		timeDemo = TD_NO;
	}
}

/*
================
idSessionLocal::DemoShot

A demoShot is a single frame demo
================
*/
void idSessionLocal::DemoShot( const char *demoName ) {
	StartRecordingRenderDemo( demoName );

	// force draw one frame
	UpdateScreen();

	StopRecordingRenderDemo();
}

/*
================
idSessionLocal::StartPlayingRenderDemo
================
*/
void idSessionLocal::StartPlayingRenderDemo( idStr demoName ) {
	if ( !demoName[0] ) {
		common->Printf( "idSessionLocal::StartPlayingRenderDemo: no name specified\n" );
		return;
	}

	// make sure localSound / GUI intro music shuts up
	sw->StopAllSounds();
	sw->PlayShaderDirectly( "", 0 );
	menuSoundWorld->StopAllSounds();
	menuSoundWorld->PlayShaderDirectly( "", 0 );

	// exit any current game
	Stop();

	// automatically put the console away
	console->Close();

	// bring up the loading screen manually, since demos won't
	// call ExecuteMapChange()
	guiLoading = uiManager->FindGui( "guis/map/loading.gui", true, false, true );
	guiLoading->SetStateString( "demo", common->GetLanguageDict()->GetString( "#str_02087" ) );
	readDemo = new idDemoFile;
	demoName.DefaultFileExtension( ".demo" );
	if ( !readDemo->OpenForReading( demoName ) ) {
		common->Printf( "couldn't open %s\n", demoName.c_str() );
		delete readDemo;
		readDemo = NULL;
		Stop();
		StartMenu();
		soundSystem->SetMute( false );
		return;
	}

	insideExecuteMapChange = true;
	UpdateScreen();
	insideExecuteMapChange = false;
	guiLoading->SetStateString( "demo", "" );

	// setup default render demo settings
	// that's default for <= Doom3 v1.1
	renderdemoVersion = 1;
	savegameVersion = 16;

	AdvanceRenderDemo( true );

	numDemoFrames = 1;

	lastDemoTic = -1;
	timeDemoStartTime = Sys_Milliseconds();
}


/*
================
idSessionLocal::TimeRenderDemo
================
*/
void idSessionLocal::TimeRenderDemo( const char *demoName, bool twice ) {
	idStr demo = demoName;

	// no sound in time demos
	soundSystem->SetMute( true );

	StartPlayingRenderDemo( demo );

	if ( twice && readDemo ) {
		// cycle through once to precache everything
		guiLoading->SetStateString( "demo", common->GetLanguageDict()->GetString( "#str_04852" ) );
		guiLoading->StateChanged( com_frameTime );
		while ( readDemo ) {
			insideExecuteMapChange = true;
			UpdateScreen();
			insideExecuteMapChange = false;
			AdvanceRenderDemo( true );
		}
		guiLoading->SetStateString( "demo", "" );
		StartPlayingRenderDemo( demo );
	}


	if ( !readDemo ) {
		return;
	}

	timeDemo = TD_YES;
}


/*
==============
SaveCmdDemoFromFile
==============
*/
void idSessionLocal::SaveCmdDemoToFile( idFile *file ) {

	mapSpawnData.serverInfo.WriteToFileHandle( file );

	for ( int i = 0 ; i < MAX_ASYNC_CLIENTS ; i++ ) {
		mapSpawnData.userInfo[i].WriteToFileHandle( file );
		mapSpawnData.persistentPlayerInfo[i].WriteToFileHandle( file );
	}

	file->Write( &mapSpawnData.mapSpawnUsercmd, sizeof( mapSpawnData.mapSpawnUsercmd ) );

	if ( numClients < 1 ) {
		numClients = 1;
	}
	file->Write( loggedUsercmds, numClients * logIndex * sizeof( loggedUsercmds[0] ) );
}

/*
==============
idSessionLocal::LoadCmdDemoFromFile
==============
*/
void idSessionLocal::LoadCmdDemoFromFile( idFile *file ) {

	mapSpawnData.serverInfo.ReadFromFileHandle( file );

	for ( int i = 0 ; i < MAX_ASYNC_CLIENTS ; i++ ) {
		mapSpawnData.userInfo[i].ReadFromFileHandle( file );
		mapSpawnData.persistentPlayerInfo[i].ReadFromFileHandle( file );
	}
	file->Read( &mapSpawnData.mapSpawnUsercmd, sizeof( mapSpawnData.mapSpawnUsercmd ) );
}

/*
==============
idSessionLocal::WriteCmdDemo

Dumps the accumulated commands for the current level.
This should still work after disconnecting from a level
==============
*/
void idSessionLocal::WriteCmdDemo( const char *demoName, bool save ) {

	if ( !demoName[0] ) {
		common->Printf( "idSessionLocal::WriteCmdDemo: no name specified\n" );
		return;
	}

	idStr statsName;
	if (save) {
		statsName = demoName;
		statsName.StripFileExtension();
		statsName.DefaultFileExtension(".stats");
	}

	common->Printf( "writing save data to %s\n", demoName );

	idFile *cmdDemoFile = fileSystem->OpenFileWrite( demoName );
	if ( !cmdDemoFile ) {
		common->Printf( "Couldn't open for writing %s\n", demoName );
		return;
	}

	if ( save ) {
		cmdDemoFile->Write( &logIndex, sizeof( logIndex ) );
	}

	SaveCmdDemoToFile( cmdDemoFile );

	if ( save ) {
		idFile *statsFile = fileSystem->OpenFileWrite( statsName );
		if ( statsFile ) {
			statsFile->Write( &statIndex, sizeof( statIndex ) );
			statsFile->Write( loggedStats, numClients * statIndex * sizeof( loggedStats[0] ) );
			fileSystem->CloseFile( statsFile );
		}
	}

	fileSystem->CloseFile( cmdDemoFile );
}

/*
===============
idSessionLocal::FinishCmdLoad
===============
*/
void idSessionLocal::FinishCmdLoad() {
}

/*
===============
idSessionLocal::StartPlayingCmdDemo
===============
*/
void idSessionLocal::StartPlayingCmdDemo(const char *demoName) {
	// exit any current game
	Stop();

	idStr fullDemoName = "demos/";
	fullDemoName += demoName;
	fullDemoName.DefaultFileExtension( ".cdemo" );
	cmdDemoFile = fileSystem->OpenFileRead(fullDemoName);

	if ( cmdDemoFile == NULL ) {
		common->Printf( "Couldn't open %s\n", fullDemoName.c_str() );
		return;
	}

	guiLoading = uiManager->FindGui( "guis/map/loading.gui", true, false, true );
	//cmdDemoFile->Read(&loadGameTime, sizeof(loadGameTime));

	LoadCmdDemoFromFile(cmdDemoFile);

	// start the map
	ExecuteMapChange();

	cmdDemoFile = fileSystem->OpenFileRead(fullDemoName);

	// have to do this twice as the execmapchange clears the cmddemofile
	LoadCmdDemoFromFile(cmdDemoFile);

	// run one frame to get the view angles correct
	RunGameTic();
}

/*
===============
idSessionLocal::TimeCmdDemo
===============
*/
void idSessionLocal::TimeCmdDemo( const char *demoName ) {
	StartPlayingCmdDemo( demoName );
	ClearWipe();
	UpdateScreen();

	int		startTime = Sys_Milliseconds();
	int		count = 0;
	int		minuteStart, minuteEnd;
	float	sec;

	// run all the frames in sequence
	minuteStart = startTime;

	while( cmdDemoFile ) {
		RunGameTic();
		count++;

		if ( count / 3600 != ( count - 1 ) / 3600 ) {
			minuteEnd = Sys_Milliseconds();
			sec = ( minuteEnd - minuteStart ) / 1000.0;
			minuteStart = minuteEnd;
			common->Printf( "minute %i took %3.1f seconds\n", count / 3600, sec );
			UpdateScreen();
		}
	}

	int		endTime = Sys_Milliseconds();
	sec = ( endTime - startTime ) / 1000.0;
	common->Printf( "%i seconds of game, replayed in %5.1f seconds\n", count / 60, sec );
}

/*
================
idSessionLocal::BeginAVICapture
================
*/
void idSessionLocal::BeginAVICapture( const char *demoName ) {
	idStr name = demoName;
	name.ExtractFileBase( aviDemoShortName );
	aviCaptureMode = true;
	aviDemoFrameCount = 0;
	aviTicStart = 0;
	sw->AVIOpen( va( "demos/%s/", aviDemoShortName.c_str() ), aviDemoShortName.c_str() );
}

/*
================
idSessionLocal::EndAVICapture
================
*/
void idSessionLocal::EndAVICapture() {
	if ( !aviCaptureMode ) {
		return;
	}

	sw->AVIClose();

	// write a .roqParam file so the demo can be converted to a roq file
	idFile *f = fileSystem->OpenFileWrite( va( "demos/%s/%s.roqParam",
		aviDemoShortName.c_str(), aviDemoShortName.c_str() ) );
	f->Printf( "INPUT_DIR demos/%s\n", aviDemoShortName.c_str() );
	f->Printf( "FILENAME demos/%s/%s.RoQ\n", aviDemoShortName.c_str(), aviDemoShortName.c_str() );
	f->Printf( "\nINPUT\n" );
	f->Printf( "%s_*.tga [00000-%05i]\n", aviDemoShortName.c_str(), (int)( aviDemoFrameCount-1 ) );
	f->Printf( "END_INPUT\n" );
	delete f;

	common->Printf( "captured %i frames for %s.\n", ( int )aviDemoFrameCount, aviDemoShortName.c_str() );

	aviCaptureMode = false;
}


/*
================
idSessionLocal::AVIRenderDemo
================
*/
void idSessionLocal::AVIRenderDemo( const char *_demoName ) {
	idStr	demoName = _demoName;	// copy off from va() buffer

	StartPlayingRenderDemo( demoName );
	if ( !readDemo ) {
		return;
	}

	BeginAVICapture( demoName.c_str() ) ;

	// I don't understand why I need to do this twice, something
	// strange with the nvidia swapbuffers?
	UpdateScreen();
}

/*
================
idSessionLocal::AVICmdDemo
================
*/
void idSessionLocal::AVICmdDemo( const char *demoName ) {
	StartPlayingCmdDemo( demoName );

	BeginAVICapture( demoName ) ;
}

/*
================
idSessionLocal::AVIGame

Start AVI recording the current game session
================
*/
void idSessionLocal::AVIGame( const char *demoName ) {
	if ( aviCaptureMode ) {
		EndAVICapture();
		return;
	}

	if ( !mapSpawned ) {
		common->Printf( "No map spawned.\n" );
	}

	if ( !demoName || !demoName[0] ) {
		idStr filename = FindUnusedFileName( "demos/game%03i.game" );
		demoName = filename.c_str();

		// write a one byte stub .game file just so the FindUnusedFileName works,
		fileSystem->WriteFile( demoName, demoName, 1 );
	}

	BeginAVICapture( demoName ) ;
}

/*
================
idSessionLocal::CompressDemoFile
================
*/
void idSessionLocal::CompressDemoFile( const char *scheme, const char *demoName ) {
	idStr	fullDemoName = "demos/";
	fullDemoName += demoName;
	fullDemoName.DefaultFileExtension( ".demo" );
	idStr compressedName = fullDemoName;
	compressedName.StripFileExtension();
	compressedName.Append( "_compressed.demo" );

	int savedCompression = cvarSystem->GetCVarInteger("com_compressDemos");
	bool savedPreload = cvarSystem->GetCVarBool("com_preloadDemos");
	cvarSystem->SetCVarBool( "com_preloadDemos", false );
	cvarSystem->SetCVarInteger("com_compressDemos", atoi(scheme) );

	idDemoFile demoread, demowrite;
	if ( !demoread.OpenForReading( fullDemoName ) ) {
		common->Printf( "Could not open %s for reading\n", fullDemoName.c_str() );
		return;
	}
	if ( !demowrite.OpenForWriting( compressedName ) ) {
		common->Printf( "Could not open %s for writing\n", compressedName.c_str() );
		demoread.Close();
		cvarSystem->SetCVarBool( "com_preloadDemos", savedPreload );
		cvarSystem->SetCVarInteger("com_compressDemos", savedCompression);
		return;
	}
	common->SetRefreshOnPrint( true );
	common->Printf( "Compressing %s to %s...\n", fullDemoName.c_str(), compressedName.c_str() );

	static const int bufferSize = 65535;
	char buffer[bufferSize];
	int bytesRead;
	while ( 0 != (bytesRead = demoread.Read( buffer, bufferSize ) ) ) {
		demowrite.Write( buffer, bytesRead );
		common->Printf( "." );
	}

	demoread.Close();
	demowrite.Close();

	cvarSystem->SetCVarBool( "com_preloadDemos", savedPreload );
	cvarSystem->SetCVarInteger("com_compressDemos", savedCompression);

	common->Printf( "Done\n" );
	common->SetRefreshOnPrint( false );

}

/*
===============
idSessionLocal::AdvanceRenderDemo
===============
*/
void idSessionLocal::AdvanceRenderDemo( bool singleFrameOnly ) {
	if ( lastDemoTic == -1 ) {
		lastDemoTic = latchedTicNumber - 1;
	}

	int skipFrames = 0;

	if ( !aviCaptureMode && !timeDemo && !singleFrameOnly ) {
		skipFrames = ( (latchedTicNumber - lastDemoTic) / USERCMD_PER_DEMO_FRAME ) - 1;
		// never skip too many frames, just let it go into slightly slow motion
		if ( skipFrames > 4 ) {
			skipFrames = 4;
		}
		lastDemoTic = latchedTicNumber - latchedTicNumber % USERCMD_PER_DEMO_FRAME;
	} else {
		// always advance a single frame with avidemo and timedemo
		lastDemoTic = latchedTicNumber;
	}

	while( skipFrames > -1 ) {
		int		ds = DS_FINISHED;

		readDemo->ReadInt( ds );
		if ( ds == DS_FINISHED ) {
			if ( numDemoFrames != 1 ) {
				// if the demo has a single frame (a demoShot), continuously replay
				// the renderView that has already been read
				Stop();
				StartMenu();
			}
			break;
		}
		if ( ds == DS_RENDER ) {
			if ( rw->ProcessDemoCommand( readDemo, &currentDemoRenderView, &demoTimeOffset ) ) {
				// a view is ready to render
				skipFrames--;
				numDemoFrames++;
			}
			continue;
		}
		if ( ds == DS_SOUND ) {
			sw->ProcessDemoCommand( readDemo );
			continue;
		}
		// appears in v1.2, with savegame format 17
		if ( ds == DS_VERSION ) {
			readDemo->ReadInt( renderdemoVersion );
			common->Printf( "reading a v%d render demo\n", renderdemoVersion );
			// set the savegameVersion to current for render demo paths that share the savegame paths
			savegameVersion = SAVEGAME_VERSION;
			continue;
		}
		common->Error( "Bad render demo token" );
	}

	if ( com_showDemo.GetBool() ) {
		common->Printf( "frame:%i DemoTic:%i latched:%i skip:%i\n", numDemoFrames, lastDemoTic, latchedTicNumber, skipFrames );
	}

}

/*
================
Session_DemoShot_f
================
*/
CONSOLE_COMMAND( demoShot, "writes a screenshot as a demo", NULL ) {
	if ( args.Argc() != 2 ) {
		idStr filename = FindUnusedFileName( "demos/shot%03i.demo" );
		sessLocal.DemoShot( filename );
	} else {
		sessLocal.DemoShot( va( "demos/shot_%s.demo", args.Argv(1) ) );
	}
}

#ifndef	ID_DEDICATED
/*
================
Session_ExitCmdDemo_f
================
*/
static void Session_ExitCmdDemo_f( const idCmdArgs &args ) {
	if ( !sessLocal.cmdDemoFile ) {
		common->Printf( "not reading from a cmdDemo\n" );
		return;
	}
	fileSystem->CloseFile( sessLocal.cmdDemoFile );
	common->Printf( "Command demo exited at logIndex %i\n", sessLocal.logIndex );
	sessLocal.cmdDemoFile = NULL;
}

/*
================
Session_RecordDemo_f
================
*/
CONSOLE_COMMAND( recordDemo, "records a demo", NULL ) {
	if ( args.Argc() != 2 ) {
		idStr filename = FindUnusedFileName( "demos/demo%03i.demo" );
		sessLocal.StartRecordingRenderDemo( filename );
	} else {
		sessLocal.StartRecordingRenderDemo( va( "demos/%s.demo", args.Argv(1) ) );
	}
}

/*
================
Session_CompressDemo_f
================
*/
CONSOLE_COMMAND( compressDemo, "compresses a demo file", idCmdSystem::ArgCompletion_DemoName ) {
	if ( args.Argc() == 2 ) {
		sessLocal.CompressDemoFile( "2", args.Argv(1) );
	} else if ( args.Argc() == 3 ) {
		sessLocal.CompressDemoFile( args.Argv(2), args.Argv(1) );
	} else {
		common->Printf( "use: CompressDemo <file> [scheme]\nscheme is the same as com_compressDemo, defaults to 2" );
	}
}

/*
================
Session_StopRecordingDemo_f
================
*/
CONSOLE_COMMAND( stopRecording, "stops demo recording", NULL ) {
	sessLocal.StopRecordingRenderDemo();
}

/*
================
Session_PlayDemo_f
================
*/
CONSOLE_COMMAND( playDemo, "plays back a demo", idCmdSystem::ArgCompletion_DemoName ) {
	if ( args.Argc() >= 2 ) {
		sessLocal.StartPlayingRenderDemo( va( "demos/%s", args.Argv(1) ) );
	}
}

/*
================
Session_TimeDemo_f
================
*/
CONSOLE_COMMAND( timeDemo, "times a demo", idCmdSystem::ArgCompletion_DemoName ) {
	if ( args.Argc() >= 2 ) {
		sessLocal.TimeRenderDemo( va( "demos/%s", args.Argv(1) ), ( args.Argc() > 2 ) );
	}
}

/*
================
Session_TimeDemoQuit_f
================
*/
CONSOLE_COMMAND( timeDemoQuit, "times a demo and quits", idCmdSystem::ArgCompletion_DemoName ) {
	sessLocal.TimeRenderDemo( va( "demos/%s", args.Argv(1) ) );
	if ( sessLocal.timeDemo == TD_YES ) {
		// this allows hardware vendors to automate some testing
		sessLocal.timeDemo = TD_YES_THEN_QUIT;
	}
}

/*
================
Session_AVIDemo_f
================
*/
CONSOLE_COMMAND( aviDemo, "writes AVIs for a demo", idCmdSystem::ArgCompletion_DemoName ) {
	sessLocal.AVIRenderDemo( va( "demos/%s", args.Argv(1) ) );
}

/*
================
Session_AVIGame_f
================
*/
CONSOLE_COMMAND( aviGame, "writes AVIs for the current game", NULL ) {
	sessLocal.AVIGame( args.Argv(1) );
}

/*
================
Session_AVICmdDemo_f
================
*/
CONSOLE_COMMAND( aviCmdDemo, "writes AVIs for a command demo", NULL ) {
	sessLocal.AVICmdDemo( args.Argv(1) );
}

/*
================
Session_WriteCmdDemo_f
================
*/
CONSOLE_COMMAND( writeCmdDemo, "writes a command demo", NULL ) {
	if ( args.Argc() == 1 ) {
		idStr	filename = FindUnusedFileName( "demos/cmdDemo%03i.cdemo" );
		sessLocal.WriteCmdDemo( filename );
	} else if ( args.Argc() == 2 ) {
		sessLocal.WriteCmdDemo( va( "demos/%s.cdemo", args.Argv( 1 ) ) );
	} else {
		common->Printf( "usage: writeCmdDemo [demoName]\n" );
	}
}

/*
================
Session_PlayCmdDemo_f
================
*/
CONSOLE_COMMAND( playCmdDemo, "plays back a command demo", NULL ) {
	sessLocal.StartPlayingCmdDemo( args.Argv(1) );
}

/*
================
Session_TimeCmdDemo_f
================
*/
CONSOLE_COMMAND( timeCmdDemo, "times a command demo", NULL ) {
	sessLocal.TimeCmdDemo( args.Argv(1) );
}
#endif