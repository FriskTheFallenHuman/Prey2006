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

idCVar	idSessionLocal::com_wipeSeconds( "com_wipeSeconds", "1", CVAR_SYSTEM, "" );
idCVar	idSessionLocal::com_numQuicksaves( "com_numQuicksaves", "4", CVAR_SYSTEM|CVAR_ARCHIVE|CVAR_INTEGER, "number of quicksaves to keep before overwriting the oldest", 1, 99 );
idCVar com_updateLoadSize( "com_updateLoadSize", "0", CVAR_BOOL | CVAR_SYSTEM | CVAR_NOCHEAT, "update the load size after loading a map" );

static idCVar g_levelloadmusic( "g_levelloadmusic", "1", CVAR_GAME | CVAR_ARCHIVE | CVAR_BOOL, "play music during level loads" );

/*
================
idSessionLocal::StartWipe

Draws and captures the current state, then starts a wipe with that image
================
*/
void idSessionLocal::StartWipe( const char *_wipeMaterial, bool hold ) {
	console->Close();

	// render the current screen into a texture for the wipe model
	renderSystem->CropRenderSize( renderSystem->GetVirtualWidth(), renderSystem->GetVirtualHeight(), true );

	Draw();

	renderSystem->CaptureRenderToImage( "_scratch");
	renderSystem->UnCrop();

	wipeMaterial = declManager->FindMaterial( _wipeMaterial, false );

	wipeStartTic = com_ticNumber;
	wipeStopTic = wipeStartTic + 1000.0f / USERCMD_MSEC * com_wipeSeconds.GetFloat();
	wipeHold = hold;
}

/*
================
idSessionLocal::CompleteWipe
================
*/
void idSessionLocal::CompleteWipe() {
	if ( com_ticNumber == 0 ) {
		// if the async thread hasn't started, we would hang here
		wipeStopTic = 0;
		UpdateScreen( true );
		return;
	}
	while ( com_ticNumber < wipeStopTic ) {
#if ID_CONSOLE_LOCK
		emptyDrawCount = 0;
#endif
		com_ticNumber++;
		UpdateScreen( true );
	}
}

/*
================
idSessionLocal::ClearWipe
================
*/
void idSessionLocal::ClearWipe( void ) {
	wipeHold = false;
	wipeStopTic = 0;
	wipeStartTic = wipeStopTic + 1;
}

/*
===============
idSessionLocal::StartNewGame
===============
*/
void idSessionLocal::StartNewGame( const char *mapName, bool devmap ) {
#ifdef	ID_DEDICATED
	common->Printf( "Dedicated servers cannot start singleplayer games.\n" );
	return;
#else
#if ID_ENFORCE_KEY
	// strict check. don't let a game start without a definitive answer
	if ( !CDKeysAreValid( true ) ) {
		bool prompt = true;
		if ( MaybeWaitOnCDKey() ) {
			// check again, maybe we just needed more time
			if ( CDKeysAreValid( true ) ) {
				// can continue directly
				prompt = false;
			}
		}
		if ( prompt ) {
			cmdSystem->BufferCommandText( CMD_EXEC_NOW, "promptKey force" );
			cmdSystem->ExecuteCommandBuffer();
		}
	}
#endif
	if ( idAsyncNetwork::server.IsActive() ) {
		common->Printf("Server running, use si_map / serverMapRestart\n");
		return;
	}
	if ( idAsyncNetwork::client.IsActive() ) {
		common->Printf("Client running, disconnect from server first\n");
		return;
	}

	// clear the userInfo so the player starts out with the defaults
	mapSpawnData.userInfo[0].Clear();
	mapSpawnData.persistentPlayerInfo[0].Clear();
	mapSpawnData.userInfo[0] = *cvarSystem->MoveCVarsToDict( CVAR_USERINFO );

	mapSpawnData.serverInfo.Clear();
	mapSpawnData.serverInfo = *cvarSystem->MoveCVarsToDict( CVAR_SERVERINFO );
	mapSpawnData.serverInfo.Set( "si_gameType", "singleplayer" );

	const char *deathwalkmap = GetDeathwalkMapName( mapName );
	mapSpawnData.serverInfo.Set( "deathwalkmap", deathwalkmap );
	mapSpawnData.serverInfo.SetBool( "shouldappendlevel", deathwalkmap && deathwalkmap[0] );

	// set the devmap key so any play testing items will be given at
	// spawn time to set approximately the right weapons and ammo
	if(devmap) {
		mapSpawnData.serverInfo.Set( "devmap", "1" );
	}

	mapSpawnData.syncedCVars.Clear();
	mapSpawnData.syncedCVars = *cvarSystem->MoveCVarsToDict( CVAR_NETWORKSYNC );

	MoveToNewMap( mapName );

	cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "exitMenu" );
#endif
}

/*
===============
idSessionLocal::GetAutoSaveName
===============
*/
idStr idSessionLocal::GetAutoSaveName( const char *mapName ) const {
	const idDecl *mapDecl = declManager->FindType( DECL_MAPDEF, mapName, false );
	const idDeclEntityDef *mapDef = static_cast<const idDeclEntityDef *>( mapDecl );
	if ( mapDef ) {
		mapName = common->GetLanguageDict()->GetString( mapDef->dict.GetString( "name", mapName ) );
	}
	// Fixme: Localization
	return va( "^3AutoSave:^0 %s", mapName );
}

/*
===============
idSessionLocal::MoveToNewMap

Leaves the existing userinfo and serverinfo
===============
*/
void idSessionLocal::MoveToNewMap( const char *mapName ) {
	mapSpawnData.serverInfo.Set( "si_map", mapName );

	ExecuteMapChange();

	if ( !mapSpawnData.serverInfo.GetBool("devmap") ) {
		// Autosave at the beginning of the level

		// DG: set an explicit savename to avoid problems with autosave names
		//     (they were translated which caused problems like all alpha labs parts
		//      getting the same filename in spanish, probably because the strings contained
		//      dots and everything behind them was cut off as "file extension".. see #305)
		idStr saveFileName = "Autosave_";
		saveFileName += mapName;
		SaveGame( GetAutoSaveName( mapName ), true, saveFileName );
	}

	SetGUI( NULL, NULL );
}

/*
===============
idSessionLocal::UnloadMap

Performs cleanup that needs to happen between maps, or when a
game is exited.
Exits with mapSpawned = false
===============
*/
void idSessionLocal::UnloadMap() {
	StopPlayingRenderDemo();

	// end the current map in the game
	if ( game ) {
		game->MapShutdown();
	}

	if ( cmdDemoFile ) {
		fileSystem->CloseFile( cmdDemoFile );
		cmdDemoFile = NULL;
	}

	if ( writeDemo ) {
		StopRecordingRenderDemo();
	}

	mapSpawned = false;
}

/*
===============
idSessionLocal::LoadLoadingGui
===============
*/
void idSessionLocal::LoadLoadingGui( const char *mapName ) {
	// load / program a gui to stay up on the screen while loading
	idStr stripped = mapName;
	stripped.StripFileExtension();
	stripped.StripPath();

	char guiMap[ MAX_STRING_CHARS ];
	idStr::Copynz( guiMap, va( "guis/map/%s.gui", stripped.c_str() ), MAX_STRING_CHARS );
	// give the gamecode a chance to override
	game->GetMapLoadingGUI( guiMap );

	if ( uiManager->CheckGui( guiMap ) ) {
		guiLoading = uiManager->FindGui( guiMap, true, false, true );
	} else {
		guiLoading = uiManager->FindGui( "guis/map/loading.gui", true, false, true );
		// bg image
		guiLoading->SetStateString( "image", idStr( "guis/assets/loading/" ) + stripped + ".tga" );
		// title
		const idDecl *mapDecl = declManager->FindType( DECL_MAPDEF, mapName, false );
		const idDeclEntityDef *mapDef = static_cast<const idDeclEntityDef *>( mapDecl );
		guiLoading->SetStateString( "friendlyname", mapDef ? 
			//mapSpawnData.serverInfo.GetBool( "devmap" ) ? mapDef->dict.GetString( "devname" )
			cvarSystem->GetCVarBool( "developer" ) ? mapDef->dict.GetString( "devname" )
			: ( common->GetLanguageDict()->GetString( mapDef->dict.GetString( "name" ) ) )
				: stripped.c_str() );
	}
	guiLoading->SetStateFloat( "map_loading", 0.0f );
}

/*
===============
idSessionLocal::GetBytesNeededForMapLoad
===============
*/
int idSessionLocal::GetBytesNeededForMapLoad( const char *mapName ) {
	const idDecl *mapDecl = declManager->FindType( DECL_MAPDEF, mapName, false );
	const idDeclEntityDef *mapDef = static_cast<const idDeclEntityDef *>( mapDecl );
	if ( mapDef ) {
		return mapDef->dict.GetInt( va("size%d", Max( 0, com_imageQuality.GetInteger() ) ) );
	} else {
		if ( com_imageQuality.GetInteger() < 2 ) {
			return 200 * 1024 * 1024;
		} else {
			return 400 * 1024 * 1024;
		}
	}
}

/*
===============
idSessionLocal::SetBytesNeededForMapLoad
===============
*/
void idSessionLocal::SetBytesNeededForMapLoad( const char *mapName, int bytesNeeded ) {
	idDecl *mapDecl = const_cast<idDecl *>(declManager->FindType( DECL_MAPDEF, mapName, false ));
	idDeclEntityDef *mapDef = static_cast<idDeclEntityDef *>( mapDecl );

	if ( com_updateLoadSize.GetBool() && mapDef ) {
		// we assume that if com_updateLoadSize is true then the file is writable

		mapDef->dict.SetInt( va("size%d", com_imageQuality.GetInteger()), bytesNeeded );

		idStr declText = "\nmapDef ";
		declText += mapDef->GetName();
		declText += " {\n";
		for (int i=0; i<mapDef->dict.GetNumKeyVals(); i++) {
			const idKeyValue *kv = mapDef->dict.GetKeyVal( i );
			if ( kv && (kv->GetKey().Cmp("classname") != 0 ) ) {
				declText += "\t\"" + kv->GetKey() + "\"\t\t\"" + kv->GetValue() + "\"\n";
			}
		}
		declText += "}";
		mapDef->SetText( declText );
		mapDef->ReplaceSourceFileText();
	}
}

/*
===============
idSessionLocal::ExecuteMapChange

Performs the initialization of a game based on mapSpawnData, used for both single
player and multiplayer, but not for renderDemos, which don't
create a game at all.
Exits with mapSpawned = true
===============
*/
void idSessionLocal::ExecuteMapChange( bool noFadeWipe ) {
	int		i;
	bool	reloadingSameMap;

	// close console and remove any prints from the notify lines
	console->Close();

	if ( IsMultiplayer() ) {
		// make sure the mp GUI isn't up, or when players get back in the
		// map, mpGame's menu and the gui will be out of sync.
		SetGUI( NULL, NULL );
	}

	// mute sound
	soundSystem->SetMute( true );

	// clear all menu sounds
	menuSoundWorld->ClearAllSoundEmitters();

	// unpause the game sound world
	// NOTE: we UnPause again later down. not sure this is needed
	if ( sw->IsPaused() ) {
		sw->UnPause();
	}

	if ( !noFadeWipe ) {
		// capture the current screen and start a wipe
		StartWipe( "wipeMaterial", true );

		// immediately complete the wipe to fade out the level transition
		// run the wipe to completion
		CompleteWipe();
	}

	// extract the map name from serverinfo
	idStr mapString = mapSpawnData.serverInfo.GetString( "si_map" );

	idStr fullMapName = "maps/";
	fullMapName += mapString;
	fullMapName.StripFileExtension();

	// shut down the existing game if it is running
	UnloadMap();

	// don't do the deferred caching if we are reloading the same map
	if ( fullMapName == currentMapName ) {
		reloadingSameMap = true;
	} else {
		reloadingSameMap = false;
		currentMapName = fullMapName;
	}

	// note which media we are going to need to load
	if ( !reloadingSameMap ) {
		declManager->BeginLevelLoad();
		renderSystem->BeginLevelLoad();
		soundSystem->BeginLevelLoad();
	}

	if ( g_levelloadmusic.GetBool() ) {
		soundSystem->SetMute( false );
		soundSystem->SetPlayingSoundWorld( menuSoundWorld );
		const idDecl *mapDecl = declManager->FindType( DECL_MAPDEF, mapString.c_str(), false );
		if ( mapDecl ) {
			const idDeclEntityDef *mapDef = static_cast<const idDeclEntityDef *>( mapDecl );
			const char *loadMusic = mapDef->dict.GetString( "snd_loadmusic" );
			if ( loadMusic && loadMusic[0] ) {
				menuSoundWorld->PlayShaderDirectly( loadMusic, 2 );
			}
		}
	}
	subtitleTextScaleInited = false; // reload subtitles's text scale

	uiManager->BeginLevelLoad();
	uiManager->Reload( true );

	// set the loading gui that we will wipe to
	LoadLoadingGui( mapString );

	// cause prints to force screen updates as a pacifier,
	// and draw the loading gui instead of game draws
	insideExecuteMapChange = true;

	// if this works out we will probably want all the sizes in a def file although this solution will
	// work for new maps etc. after the first load. we can also drop the sizes into the default.cfg
	fileSystem->ResetReadCount();
	if ( !reloadingSameMap  ) {
		bytesNeededForMapLoad = GetBytesNeededForMapLoad( mapString.c_str() );
	} else {
		bytesNeededForMapLoad = 30 * 1024 * 1024;
	}

	ClearWipe();

	// let the loading gui spin for 1 second to animate out
	ShowLoadingGui();

	// note any warning prints that happen during the load process
	common->ClearWarnings( mapString );

	// release the mouse cursor
	// before we do this potentially long operation
	Sys_GrabMouseCursor( false );

	// if net play, we get the number of clients during mapSpawnInfo processing
	if ( !idAsyncNetwork::IsActive() ) {
		numClients = 1;
	}

	int start = Sys_Milliseconds();

	common->Printf( "----- Map Initialization -----\n" );
	common->Printf( "Map: %s\n", mapString.c_str() );

	// let the renderSystem load all the geometry
	if ( !rw->InitFromMap( fullMapName ) ) {
		common->Error( "couldn't load %s", fullMapName.c_str() );
	}

	// for the synchronous networking we needed to roll the angles over from
	// level to level, but now we can just clear everything
	usercmdGen->InitForNewMap();
	memset( &mapSpawnData.mapSpawnUsercmd, 0, sizeof( mapSpawnData.mapSpawnUsercmd ) );

	// set the user info
	for ( i = 0; i < numClients; i++ ) {
		game->SetUserInfo( i, mapSpawnData.userInfo[i], idAsyncNetwork::client.IsActive(), false );
		game->SetPersistentPlayerInfo( i, mapSpawnData.persistentPlayerInfo[i] );
	}

	// load and spawn all other entities ( from a savegame possibly )
	if ( loadingSaveGame && savegameFile ) {
		if ( game->InitFromSaveGame( fullMapName + ".map", rw, sw, savegameFile ) == false ) {
			// If the loadgame failed, restart the map with the player persistent data
			loadingSaveGame = false;
			fileSystem->CloseFile( savegameFile );
			savegameFile = NULL;

			common->Warning( "WARNING: Loading savegame failed, will restart the map with the player persistent data!" );

			game->SetServerInfo( mapSpawnData.serverInfo );
			game->InitFromNewMap( fullMapName + ".map", rw, sw, idAsyncNetwork::server.IsActive(), idAsyncNetwork::client.IsActive(), Sys_Milliseconds() );
		}
	} else {
		game->SetServerInfo( mapSpawnData.serverInfo );
		game->InitFromNewMap( fullMapName + ".map", rw, sw, idAsyncNetwork::server.IsActive(), idAsyncNetwork::client.IsActive(), Sys_Milliseconds() );
	}

	if ( !idAsyncNetwork::IsActive() && !loadingSaveGame ) {
		// spawn players
		for ( i = 0; i < numClients; i++ ) {
			game->SpawnPlayer( i );
		}
	}

	// actually purge/load the media
	if ( !reloadingSameMap ) {
		renderSystem->EndLevelLoad();
		soundSystem->EndLevelLoad( mapString.c_str() );
		declManager->EndLevelLoad();
		SetBytesNeededForMapLoad( mapString.c_str(), fileSystem->GetReadCount() );
	}
	uiManager->EndLevelLoad();

	if ( !idAsyncNetwork::IsActive() && !loadingSaveGame ) {
		// run a few frames to allow everything to settle
		for ( i = 0; i < 10; i++ ) {
			game->RunFrame( mapSpawnData.mapSpawnUsercmd );
		}
	}

	int	msec = Sys_Milliseconds() - start;
	common->Printf( "%6d msec to load %s\n", msec, mapString.c_str() );

	// let the renderSystem generate interactions now that everything is spawned
	rw->GenerateAllInteractions();

	common->PrintWarnings();

	if ( guiLoading && bytesNeededForMapLoad ) {
		float pct = guiLoading->State().GetFloat( "map_loading" );
		if ( pct < 0.0f ) {
			pct = 0.0f;
		}
		while ( pct < 1.0f ) {
			guiLoading->SetStateFloat( "map_loading", pct );
			guiLoading->StateChanged( com_frameTime );
			Sys_GenerateEvents();
			UpdateScreen();
			pct += 0.05f;
		}
	}

	// capture the current screen and start a wipe
	StartWipe( "wipe2Material" );

	usercmdGen->Clear();

	// start saving commands for possible writeCmdDemo usage
	logIndex = 0;
	statIndex = 0;
	lastSaveIndex = 0;

	// don't bother spinning over all the tics we spent loading
	lastGameTic = latchedTicNumber = com_ticNumber;

	// remove any prints from the notify lines
	console->ClearNotifyLines();

	// stop drawing the laoding screen
	insideExecuteMapChange = false;

	Sys_SetPhysicalWorkMemory( -1, -1 );

	// set the game sound world for playback
	soundSystem->SetPlayingSoundWorld( sw );

	// when loading a save game the sound is paused
	if ( sw->IsPaused() ) {
		// unpause the game sound world
		sw->UnPause();
	}

	// restart entity sound playback
	soundSystem->SetMute( false );

	// we are valid for game draws now
	mapSpawned = true;
	Sys_ClearEvents();
}

/*
===============
idSessionLocal::ScrubSaveGameFileName

Turns a bad file name into a good one or your money back
===============
*/
void idSessionLocal::ScrubSaveGameFileName( idStr &saveFileName ) const {
	int i;
	idStr inFileName;

	inFileName = saveFileName;
	inFileName.RemoveColors();
	inFileName.StripFileExtension();

	saveFileName.Clear();

	int len = inFileName.Length();
	for ( i = 0; i < len; i++ ) {
		if ( strchr( "',.~!@#$%^&*()[]{}<>\\|/=?+;:-\'\"", inFileName[i] ) ) {
			// random junk
			saveFileName += '_';
		} else if ( (const unsigned char)inFileName[i] >= 128 ) {
			// high ascii chars
			saveFileName += '_';
		} else if ( inFileName[i] == ' ' ) {
			saveFileName += '_';
		} else {
			saveFileName += inFileName[i];
		}
	}
}

/*
===============
idSessionLocal::SaveGame
===============
*/
bool idSessionLocal::SaveGame( const char *saveName, bool autosave, const char* saveFileName ) {
#ifdef	ID_DEDICATED
	common->Printf( "Dedicated servers cannot save games.\n" );
	return false;
#else
	int i;
	idStr previewFile, descriptionFile, mapName;
	// DG: support setting an explicit savename to avoid problems with autosave names
	idStr gameFile = (saveFileName != NULL) ? saveFileName : saveName;

	if ( !mapSpawned ) {
		common->Printf( "Not playing a game.\n" );
		return false;
	}

	if ( IsMultiplayer() ) {
		common->Printf( "Can't save during net play.\n" );
		return false;
	}

	if ( game->GetPersistentPlayerInfo( 0 ).GetInt( "health" ) <= 0 ) {
		MessageBox( MSG_OK, common->GetLanguageDict()->GetString ( "#str_04311" ), common->GetLanguageDict()->GetString ( "#str_04312" ), true );
		common->Printf( "You must be alive to save the game\n" );
		return false;
	}

	if ( Sys_GetDriveFreeSpace( cvarSystem->GetCVarString( "fs_savepath" ) ) < 25 ) {
		MessageBox( MSG_OK, common->GetLanguageDict()->GetString ( "#str_04313" ), common->GetLanguageDict()->GetString ( "#str_04314" ), true );
		common->Printf( "Not enough drive space to save the game\n" );
		return false;
	}

	idSoundWorld *pauseWorld = soundSystem->GetPlayingSoundWorld();
	if ( pauseWorld ) {
		pauseWorld->Pause();
		soundSystem->SetPlayingSoundWorld( NULL );
	}

	// setup up filenames and paths
	ScrubSaveGameFileName( gameFile );

	gameFile = "savegames/" + gameFile;
	gameFile.SetFileExtension( ".save" );

	previewFile = gameFile;
	previewFile.SetFileExtension( ".tga" );

	descriptionFile = gameFile;
	descriptionFile.SetFileExtension( ".txt" );

	// Open savegame file
	idFile *fileOut = fileSystem->OpenFileWrite( gameFile );
	if ( fileOut == NULL ) {
		common->Warning( "Failed to open save file '%s'\n", gameFile.c_str() );
		if ( pauseWorld ) {
			soundSystem->SetPlayingSoundWorld( pauseWorld );
			pauseWorld->UnPause();
		}
		return false;
	}

	// Write SaveGame Header:
	// Game Name / Version / Map Name / Persistant Player Info

	// game
	const char *gamename = GAME_NAME;
	fileOut->WriteString( gamename );

	// version
	fileOut->WriteInt( SAVEGAME_VERSION );

	// map
	mapName = mapSpawnData.serverInfo.GetString( "si_map" );
	fileOut->WriteString( mapName );

	// persistent player info
	for ( i = 0; i < MAX_ASYNC_CLIENTS; i++ ) {
		mapSpawnData.persistentPlayerInfo[i] = game->GetPersistentPlayerInfo( i );
		mapSpawnData.persistentPlayerInfo[i].WriteToFileHandle( fileOut );
	}

	// let the game save its state
	game->SaveGame( fileOut );

	// close the sava game file
	fileSystem->CloseFile( fileOut );

	// Write screenshot
	if ( !autosave ) {
		renderSystem->CropRenderSize( 320, 240, false );
		game->Draw( 0 );
		renderSystem->CaptureRenderToFile( previewFile, true );
		renderSystem->UnCrop();
	}

	// Write description, which is just a text file with
	// the unclean save name on line 1, map name on line 2, screenshot on line 3
	idFile *fileDesc = fileSystem->OpenFileWrite( descriptionFile );
	if ( fileDesc == NULL ) {
		common->Warning( "Failed to open description file '%s'\n", descriptionFile.c_str() );
		if ( pauseWorld ) {
			soundSystem->SetPlayingSoundWorld( pauseWorld );
			pauseWorld->UnPause();
		}
		return false;
	}

	idStr description = saveName;
	description.Replace( "\\", "\\\\" );
	description.Replace( "\"", "\\\"" );

	const idDeclEntityDef *mapDef = static_cast<const idDeclEntityDef *>(declManager->FindType( DECL_MAPDEF, mapName, false ));
	if ( mapDef ) {
		mapName = common->GetLanguageDict()->GetString( mapDef->dict.GetString( "name", mapName ) );
	}

	fileDesc->Printf( "\"%s\"\n", description.c_str() );
	fileDesc->Printf( "\"%s\"\n", mapName.c_str());

	if ( autosave ) {
		idStr sshot = mapSpawnData.serverInfo.GetString( "si_map" );
		sshot.StripPath();
		sshot.StripFileExtension();
		fileDesc->Printf( "\"guis/assets/loading/%s\"\n", sshot.c_str() );
	} else {
		fileDesc->Printf( "\"\"\n" );
	}

	fileSystem->CloseFile( fileDesc );

	if ( pauseWorld ) {
		soundSystem->SetPlayingSoundWorld( pauseWorld );
		pauseWorld->UnPause();
	}

	syncNextGameFrame = true;


	return true;
#endif
}

/*
===============
idSessionLocal::LoadGame
===============
*/
bool idSessionLocal::LoadGame( const char *saveName ) {
#ifdef	ID_DEDICATED
	common->Printf( "Dedicated servers cannot load games.\n" );
	return false;
#else
	int i;
	idStr in, loadFile, saveMap, gamename;

	if ( IsMultiplayer() ) {
		common->Printf( "Can't load during net play.\n" );
		return false;
	}

	//Hide the dialog box if it is up.
	StopBox();

	loadFile = saveName;
	ScrubSaveGameFileName( loadFile );
	loadFile.SetFileExtension( ".save" );

	in = "savegames/";
	in += loadFile;

	// Open savegame file
	// only allow loads from the game directory because we don't want a base game to load
	idStr game = cvarSystem->GetCVarString( "fs_game" );
	savegameFile = fileSystem->OpenFileRead( in, true, game.Length() ? game : NULL );

	if ( savegameFile == NULL ) {
		common->Warning( "Couldn't open savegame file %s", in.c_str() );
		return false;
	}

	loadingSaveGame = true;

	// Read in save game header
	// Game Name / Version / Map Name / Persistant Player Info

	// game
	savegameFile->ReadString( gamename );

	// if this isn't a savegame for the correct game, abort loadgame
	if ( ! (gamename == GAME_NAME || gamename == "DOOM 3") ) {
		common->Warning( "Attempted to load an invalid savegame: %s", in.c_str() );

		loadingSaveGame = false;
		fileSystem->CloseFile( savegameFile );
		savegameFile = NULL;
		return false;
	}

	// version
	savegameFile->ReadInt( savegameVersion );

	// map
	savegameFile->ReadString( saveMap );

	// persistent player info
	for ( i = 0; i < MAX_ASYNC_CLIENTS; i++ ) {
		mapSpawnData.persistentPlayerInfo[i].ReadFromFileHandle( savegameFile );
	}

	// check the version, if it doesn't match, cancel the loadgame,
	// but still load the map with the persistant playerInfo from the header
	// so that the player doesn't lose too much progress.
	if ( savegameVersion != SAVEGAME_VERSION &&
		 !( savegameVersion == 16 && SAVEGAME_VERSION == 17 ) ) {	// handle savegame v16 in v17
		common->Warning( "Savegame Version mismatch: aborting loadgame and starting level with persistent data" );
		loadingSaveGame = false;
		fileSystem->CloseFile( savegameFile );
		savegameFile = NULL;
	}

	common->DPrintf( "loading a v%d savegame\n", savegameVersion );

	if ( saveMap.Length() > 0 ) {

		// Start loading map
		mapSpawnData.serverInfo.Clear();

		mapSpawnData.serverInfo = *cvarSystem->MoveCVarsToDict( CVAR_SERVERINFO );
		mapSpawnData.serverInfo.Set( "si_gameType", "singleplayer" );

		mapSpawnData.serverInfo.Set( "si_map", saveMap );

		const char *deathwalkmap = GetDeathwalkMapName( saveMap );
		mapSpawnData.serverInfo.Set( "deathwalkmap", deathwalkmap );
		mapSpawnData.serverInfo.SetBool( "shouldappendlevel", deathwalkmap && deathwalkmap[0] );

		mapSpawnData.syncedCVars.Clear();
		mapSpawnData.syncedCVars = *cvarSystem->MoveCVarsToDict( CVAR_NETWORKSYNC );

		mapSpawnData.mapSpawnUsercmd[0] = usercmdGen->TicCmd( latchedTicNumber );
		// make sure no buttons are pressed
		mapSpawnData.mapSpawnUsercmd[0].buttons = 0;

		ExecuteMapChange();

		SetGUI( NULL, NULL );
	}

	if ( loadingSaveGame ) {
		fileSystem->CloseFile( savegameFile );
		loadingSaveGame = false;
		savegameFile = NULL;
	}

	return true;
#endif
}

/*
===============
idSessionLocal::QuickSave
===============
*/
bool idSessionLocal::QuickSave( void ) {
	idStr saveName = common->GetLanguageDict()->GetString( "#str_07178" );

	idStr saveFilePathBase = saveName;
	ScrubSaveGameFileName( saveFilePathBase );
	saveFilePathBase = "savegames/" + saveFilePathBase;

	const char* game = cvarSystem->GetCVarString( "fs_game" );
	if ( game != NULL && game[0] == '\0' ) {
		game = NULL;
	}

	const int maxNum = com_numQuicksaves.GetInteger();
	int indexToUse = 1;
	ID_TIME_T oldestTime = 0;
	for( int i = 1; i <= maxNum; ++i ) {
		idStr saveFilePath = saveFilePathBase;
		if ( i > 1 ) {
			// the first one is just called "QuickSave" without a number, like before.
			// the others are called "QuickSave2" "QuickSave3" etc
			saveFilePath += i;
		}
		saveFilePath.SetFileExtension( ".save" );

		idFile *f = fileSystem->OpenFileRead( saveFilePath, true, game );
		if ( f == NULL ) {
			// this savegame doesn't exist yet => we can use this index for the name
			indexToUse = i;
			break;
		} else {
			ID_TIME_T ts = f->Timestamp();
			assert( ts != 0 );
			if ( ts < oldestTime || oldestTime == 0 ) {
				// this is the oldest quicksave we found so far => a candidate to be overwritten
				indexToUse = i;
				oldestTime = ts;
			}
			delete f;
		}
	}

	if ( indexToUse > 1 ) {
		saveName += indexToUse;
	}

	if ( SaveGame( saveName ) ) {
		common->Printf( "%s\n", saveName.c_str() );
		return true;
	}
	return false;
}

/*
===============
idSessionLocal::QuickLoad
===============
*/
bool idSessionLocal::QuickLoad( void ) {
	idStr saveName = common->GetLanguageDict()->GetString( "#str_07178" );

	idStr saveFilePathBase = saveName;
	ScrubSaveGameFileName( saveFilePathBase );
	saveFilePathBase = "savegames/" + saveFilePathBase;

	const char* game = cvarSystem->GetCVarString( "fs_game" );
	if ( game != NULL && game[0] == '\0' ) {
		game = NULL;
	}

	// find the newest QuickSave (or QuickSave2, QuickSave3, ...)
	const int maxNum = com_numQuicksaves.GetInteger();
	int indexToUse = 1;
	ID_TIME_T newestTime = 0;
	for( int i = 1; i <= maxNum; ++i ) {
		idStr saveFilePath = saveFilePathBase;
		if ( i > 1 ) {
			// the first one is just called "QuickSave" without a number, like before.
			// the others are called "QuickSave2" "QuickSave3" etc
			saveFilePath += i;
		}
		saveFilePath.SetFileExtension( ".save" );

		idFile *f = fileSystem->OpenFileRead( saveFilePath, true, game );
		if ( f != NULL ) {
			ID_TIME_T ts = f->Timestamp();
			assert( ts != 0 );
			if ( ts > newestTime ) {
				indexToUse = i;
				newestTime = ts;
			}
			delete f;
		}
	}

	if ( indexToUse > 1 ) {
		saveName += indexToUse;
	}

	return sessLocal.LoadGame( saveName );
}


/*
===============
LoadGame_f
===============
*/
CONSOLE_COMMAND_SHIP( loadGame, "loads a game", idCmdSystem::ArgCompletion_SaveGame ) {
	console->Close();
	if ( args.Argc() < 2 || idStr::Icmp(args.Argv(1), "quick" ) == 0 ) {
		sessLocal.QuickLoad();
	} else {
		sessLocal.LoadGame( args.Argv(1) );
	}
}

/*
===============
SaveGame_f
===============
*/
CONSOLE_COMMAND_SHIP( saveGame, "saves a game", NULL ) {
	if ( args.Argc() < 2 || idStr::Icmp( args.Argv(1), "quick" ) == 0 ) {
		sessLocal.QuickSave();
	} else {
		if ( sessLocal.SaveGame( args.Argv(1) ) ) {
			common->Printf( "Saved %s\n", args.Argv(1) );
		}
	}
}

#ifndef	ID_DEDICATED
/*
==================
Session_Map_f

Load a map
==================
*/
CONSOLE_COMMAND_SHIP( map, "loads a map", idCmdSystem::ArgCompletion_MapName ) {
	idStr		map, string;
	findFile_t	ff;
	idCmdArgs	rl_args;

	map = args.Argv(1);
	if ( !map.Length() ) {
		// DG: if the map command is called without any arguments, print the current map
		// TODO: could check whether we're currently in a game, otherwise the last loaded
		//       map is printed.. but OTOH, who cares
		const char* curmap = sessLocal.mapSpawnData.serverInfo.GetString( "si_map" );
		if ( curmap[0] != '\0' ) {
			common->Printf( "Current Map: %s\n", curmap );
		}
		return;
	}
	map.StripFileExtension();

	// make sure the level exists before trying to change, so that
	// a typo at the server console won't end the game
	// handle addon packs through reloadEngine
	sprintf( string, "maps/%s.map", map.c_str() );
	ff = fileSystem->FindFile( string, true );
	switch ( ff ) {
	case FIND_NO:
		common->Printf( "Can't find map %s\n", string.c_str() );
		return;
	case FIND_ADDON:
		common->Printf( "map %s is in an addon pak - reloading\n", string.c_str() );
		rl_args.AppendArg( "map" );
		rl_args.AppendArg( map );
		cmdSystem->SetupReloadEngine( rl_args );
		return;
	default:
		break;
	}

	cvarSystem->SetCVarBool( "developer", false );
	sessLocal.StartNewGame( map, true );
}

/*
==================
Session_RestartMap_f

Restart the server on the same map
==================
*/
CONSOLE_COMMAND_SHIP( restartMap, "restarts the current map", NULL ) {
	/*if (g_demoMode.GetBool())*/ {
		cmdSystem->AppendCommandText( va( "devmap %s %d\n", sessLocal.GetCurrentMapName(), 0 ) );
	}
}


/*
==================
Session_DevMap_f

Loads a map in developer mode
==================
*/
CONSOLE_COMMAND_SHIP( devmap, "loads a map in developer mode", idCmdSystem::ArgCompletion_MapName )  {
	idStr map, string;
	findFile_t	ff;
	idCmdArgs	rl_args;

	map = args.Argv(1);
	if ( !map.Length() ) {
		return;
	}
	map.StripFileExtension();

	// make sure the level exists before trying to change, so that
	// a typo at the server console won't end the game
	// handle addon packs through reloadEngine
	sprintf( string, "maps/%s.map", map.c_str() );
	ff = fileSystem->FindFile( string, true );
	switch ( ff ) {
	case FIND_NO:
		common->Printf( "Can't find map %s\n", string.c_str() );
		return;
	case FIND_ADDON:
		common->Printf( "map %s is in an addon pak - reloading\n", string.c_str() );
		rl_args.AppendArg( "devmap" );
		rl_args.AppendArg( map );
		cmdSystem->SetupReloadEngine( rl_args );
		return;
	default:
		break;
	}

	cvarSystem->SetCVarBool( "developer", true );
	sessLocal.StartNewGame( map, true );
}

/*
==================
Session_TestMap_f

Tests a map
==================
*/
CONSOLE_COMMAND( testmap, "tests a map", idCmdSystem::ArgCompletion_MapName ) {
	idStr map, string;

	map = args.Argv(1);
	if ( !map.Length() ) {
		return;
	}
	map.StripFileExtension();

	cmdSystem->BufferCommandText( CMD_EXEC_NOW, "disconnect" );

	sprintf( string, "dmap maps/%s.map", map.c_str() );
	cmdSystem->BufferCommandText( CMD_EXEC_NOW, string );

	sprintf( string, "devmap %s", map.c_str() );
	cmdSystem->BufferCommandText( CMD_EXEC_NOW, string );
}

/*
================
Session_ExitMenu_f
================
*/
CONSOLE_COMMAND_SHIP( exitMenu, "exit menu", NULL ) {
	sessLocal.ExitMenu();
}
#endif