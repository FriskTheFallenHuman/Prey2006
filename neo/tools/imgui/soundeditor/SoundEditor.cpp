/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.
Copyright (C) 2015 Daniel Gibson
Copyright (C) 2020-2023 Robert Beckebans

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

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU
General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "../ImGuiTools.h"
#pragma hdrstop

namespace ImGuiTools
{

SoundEditor::SoundEditor()
{
	Reset();
}

SoundEditor& SoundEditor::Instance()
{
	static SoundEditor instance;
	return instance;
}

void SoundEditor::Init( const idDict* dict, idEntity* speaker )
{
	Reset();

	if( soundShaders.Num() == 0 )
	{
		AddSounds();
	}
	if( soundFiles.Num() == 0 )
	{
		AddWaves();
	}

	speakerEntity = speaker;

	if( dict != NULL )
	{
		// if we have a speaker entity, fetch its origin for the title
		if( speaker != NULL )
		{
			gameEdit->EntityGetOrigin( speaker, entityPos );
		}
		const char* name = dict->GetString( "name", NULL );
		entityName		 = name ? name : gameEdit->GetUniqueEntityName( "speaker" );
		title			 = idStr::Format( "Sound Editor: %s at (%s)###SoundEditor", entityName.c_str(), entityPos.ToString() );

		Set( dict );

		// populate sounds lists
		AddGroups();
		AddSpeakers();
		AddInUseSounds();
	}
}

void SoundEditor::ReInit( const idDict* dict, idEntity* speaker )
{
	// TODO: if the soundeditor is currently shown, show a warning first about saving current changes to the last speaker?
	Instance().Init( dict, speaker );
}

void SoundEditor::Reset()
{
	title = "Sound Editor";
	strShader.Clear();
	playSound.Clear();
	fVolume	   = 0.0f;
	fMin	   = 1.0f;
	fMax	   = 10.0f;
	bPlay	   = true;
	bTriggered = false;
	bOmni	   = false;
	strGroup.Clear();
	bGroupOnly	= false;
	bOcclusion	= false;
	leadThrough = 0.0f;
	plain		= false;
	random		= 0.0f;
	wait		= 0.0f;
	shakes		= 0.0f;
	looping		= true;
	unclamped	= false;

	autoRefresh	   = true;
	didAutoRefresh = false;

	dropErrorBuf[0] = '\0';
	showDropError	= false;

	soundShaders.Clear();
	soundFiles.Clear();
	groupsList.Clear();
	speakersList.Clear();
	inUseSounds.Clear();

	selectedGroupIndex	 = -1;
	selectedSpeakerIndex = -1;
	shaderBuf[0]		 = '\0';
	groupBuf[0]			 = '\0';
	volumeBuf[0]		 = '\0';
	newSpeakerBuf[0]	 = '\0';

	speakerEntity = NULL;
	entityName.Clear();
}

void SoundEditor::Set( const idDict* dict )
{
	if( dict == NULL )
	{
		return;
	}
	fVolume		= dict->GetFloat( "s_volume", "0" );
	fMin		= dict->GetFloat( "s_mindistance", "1" );
	fMax		= dict->GetFloat( "s_maxdistance", "10" );
	leadThrough = dict->GetFloat( "s_leadthrough", "0.1" );
	plain		= dict->GetBool( "s_plain" );
	strShader	= dict->GetString( "s_shader" );
	strGroup	= dict->GetString( "soundgroup" );
	bOmni		= dict->GetInt( "s_omni", "-1" );
	bOcclusion	= dict->GetBool( "s_occlusion", "0" );
	bTriggered	= dict->GetInt( "s_waitfortrigger", "-1" );
	random		= dict->GetFloat( "random" );
	wait		= dict->GetFloat( "wait" );
	shakes		= dict->GetFloat( "s_shakes" );
	looping		= dict->GetBool( "s_looping" );
	unclamped	= dict->GetBool( "s_unclamped" );

	idStr::Copynz( shaderBuf, strShader.c_str(), sizeof( shaderBuf ) );
	idStr::Copynz( groupBuf, strGroup.c_str(), sizeof( groupBuf ) );
	idStr::snPrintf( volumeBuf, sizeof( volumeBuf ), "%.2f", fVolume );
}

void SoundEditor::AddSounds()
{
	soundShaders.Clear();
	int count = declManager->GetNumDecls( DECL_SOUND );
	for( int i = 0; i < count; i++ )
	{
		const idSoundShader* s = declManager->SoundByIndex( i, false );
		if( s )
		{
			soundShaders.Append( s->GetName() );
		}
	}
	soundShaders.Sort();
}

void SoundEditor::AddWaves()
{
	soundFiles.Clear();
	idFileList* files = fileSystem->ListFilesTree( "sound", ".wav|.ogg", true );
	if( files )
	{
		for( int i = 0; i < files->GetNumFiles(); i++ )
		{
			idStr f = files->GetFile( i );
			soundFiles.Append( f );
		}
		fileSystem->FreeFileList( files );
	}
	soundFiles.Sort();
}

void SoundEditor::AddInUseSounds()
{
	inUseSounds.Clear();
	idList<const char*> list;
	list.SetNum( 512 );
	int		  count = gameEdit->MapGetEntitiesMatchingClassWithString( "speaker", "", list.Ptr(), list.Num() );
	idStrList list2;
	for( int i = 0; i < count; i++ )
	{
		const idDict* dict = gameEdit->MapGetEntityDict( list[i] );
		if( dict )
		{
			const char* p = dict->GetString( "s_shader" );
			if( p && *p )
			{
				list2.AddUnique( p );
			}
		}
	}
	list2.Sort();
	for( int i = 0; i < list2.Num(); i++ )
	{
		inUseSounds.Append( list2[i] );
	}
}

bool SoundEditor::DropSpeaker( const char* spawnName )
{
	idAngles viewAngles;
	idVec3	 org;

	gameEdit->PlayerGetViewAngles( viewAngles );
	gameEdit->PlayerGetEyePosition( org );
	org += idAngles( 0, viewAngles.yaw, 0 ).ToForward() * 80 + idVec3( 0, 0, 1 );

	idDict args;
	args.Set( "origin", org.ToString() );
	args.Set( "classname", "speaker" );
	args.Set( "angle", va( "%f", viewAngles.yaw + 180 ) );
	args.Set( "s_shader", strShader );
	args.Set( "s_looping", "1" );
	args.Set( "s_shakes", "0" );

	idStr name;
	if( spawnName && spawnName[0] )
	{
		name = spawnName;
	}
	else if( newSpeakerBuf[0] != '\0' )
	{
		name = newSpeakerBuf;
	}
	else
	{
		name = gameEdit->GetUniqueEntityName( "speaker" );
	}
	args.Set( "name", name );

	idEntity* ent = NULL;
	gameEdit->SpawnEntityDef( args, &ent );
	if( ent )
	{
		gameEdit->EntityUpdateChangeableSpawnArgs( ent, NULL );
		gameEdit->ClearEntitySelection();
		gameEdit->AddSelectedEntity( ent );
	}

	// add to map in any case (legacy dialog did this)
	gameEdit->MapAddEntity( &args );

	const idDict* dict = gameEdit->MapGetEntityDict( args.GetString( "name" ) );
	if( dict )
	{
		Set( dict );
	}
	AddGroups();
	AddSpeakers();
	AddInUseSounds();

	// consider spawn successful if either a runtime entity was returned or the map contains the entity
	bool ok = ( ent != NULL ) || ( dict != NULL );
	return ok;
}

void SoundEditor::DeleteSelectedSpeakers()
{
	idList<idEntity*> list;
	list.SetNum( 128 );
	int count = gameEdit->GetSelectedEntities( list.Ptr(), list.Num() );
	list.SetNum( count );

	bool removed = false;
	if( count )
	{
		for( int i = 0; i < count; i++ )
		{
			const idDict* dict = gameEdit->EntityGetSpawnArgs( list[i] );
			if( dict == NULL )
			{
				continue;
			}
			const char*	  name	  = dict->GetString( "name" );
			const idDict* mapdict = gameEdit->MapGetEntityDict( name );
			if( mapdict )
			{
				gameEdit->MapRemoveEntity( name );
				idEntity* gameEnt = gameEdit->FindEntity( name );
				if( gameEnt )
				{
					gameEdit->EntityStopSound( gameEnt );
					gameEdit->EntityDelete( gameEnt );
					removed = true;
				}
			}
		}
	}
	if( removed )
	{
		AddGroups();
		AddSpeakers();
		AddInUseSounds();
	}
}

void SoundEditor::AddGroups()
{
	// nothing stored here, will be fetched dynamically in Draw when needed
}

void SoundEditor::AddSpeakers()
{
	// same as groups, fetch on demand
}

void SoundEditor::ApplyChanges( bool volumeOnly )
{
	idList<idEntity*> list;
	list.SetNum( 128 );
	int count = gameEdit->GetSelectedEntities( list.Ptr(), list.Num() );
	list.SetNum( count );

	if( count )
	{
		for( int i = 0; i < count; i++ )
		{
			const idDict* dict = gameEdit->EntityGetSpawnArgs( list[i] );
			if( dict == NULL )
			{
				continue;
			}
			const char*	  name	  = dict->GetString( "name" );
			const idDict* mapDict = gameEdit->MapGetEntityDict( name );
			if( mapDict )
			{
				if( volumeOnly )
				{
					float cur = mapDict->GetFloat( "s_volume" );
					cur += fVolume;
					gameEdit->MapSetEntityKeyVal( name, "s_volume", va( "%f", cur ) );
					gameEdit->MapSetEntityKeyVal( name, "s_justVolume", "1" );
					gameEdit->EntityUpdateChangeableSpawnArgs( list[i], mapDict );
				}
				else
				{
					idDict src;
					src.SetFloat( "s_volume", mapDict->GetFloat( "s_volume" ) );
					src.SetFloat( "s_mindistance", fMin );
					src.SetFloat( "s_maxdistance", fMax );
					src.Set( "s_shader", strShader );
					src.Set( "soundgroup", strGroup );
					src.SetInt( "s_omni", bOmni );
					src.SetBool( "s_occlusion", bOcclusion );
					src.SetInt( "s_waitfortrigger", bTriggered );
					src.SetBool( "s_looping", looping );
					src.SetBool( "s_unclamped", unclamped );
					src.SetFloat( "s_shakes", shakes );
					src.SetFloat( "wait", wait );
					src.SetFloat( "random", random );

					gameEdit->MapCopyDictToEntity( name, &src );
					gameEdit->EntityUpdateChangeableSpawnArgs( list[i], mapDict );
				}
			}
		}
	}
}

void SoundEditor::Draw()
{
	showTool						   = isShown;
	bool			 openedDropSpeaker = false;
	ImGuiWindowFlags wflags			   = ImGuiWindowFlags_MenuBar;

	if( ImGui::Begin( title.c_str(), &showTool, wflags ) )
	{
		if( ImGui::BeginMenuBar() )
		{
			if( ImGui::BeginMenu( "File" ) )
			{
				if( ImGui::MenuItem( "Refresh Lists" ) )
				{
					AddSounds();
					AddWaves();
					AddGroups();
					AddSpeakers();
					AddInUseSounds();
				}

				bool toggled = ImGui::MenuItem( "Auto Refresh", NULL, &autoRefresh );
				if( toggled && autoRefresh )
				{
					// only reset the one-shot flag when the user actually enabled it
					didAutoRefresh = false;
				}

				ImGui::Separator();

				if( ImGui::MenuItem( "Exit" ) )
				{
					Exit();
				}
				ImGui::EndMenu();
			}

			if( ImGui::BeginMenu( "Edit" ) )
			{
				if( ImGui::MenuItem( "Apply" ) )
				{
					ApplyChanges( false );
				}
				if( ImGui::MenuItem( "Save Map" ) )
				{
					gameEdit->MapSave();
				}
				ImGui::EndMenu();
			}

			if( ImGui::BeginMenu( "Speakers" ) )
			{
				if( ImGui::MenuItem( "Drop Speaker" ) )
				{
					openedDropSpeaker = true;
				}
				if( ImGui::MenuItem( "Delete Selected" ) )
				{
					DeleteSelectedSpeakers();
				}
				if( ImGui::MenuItem( "Refresh Speakers" ) )
				{
					AddSpeakers();
					AddInUseSounds();
				}
				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}

		if( openedDropSpeaker )
		{
			dropErrorBuf[0] = '\0';
			showDropError	= false;

			ImGui::OpenPopup( "Drop Speaker" );
		}

		// If auto-refresh is enabled, perform one refresh when editor is opened
		if( autoRefresh && !didAutoRefresh )
		{
			AddSounds();
			AddWaves();
			AddGroups();
			AddSpeakers();
			AddInUseSounds();
			didAutoRefresh = true;
		}

		// Drop Speaker modal popup
		if( ImGui::BeginPopupModal( "Drop Speaker", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDecoration ) )
		{
			ImGui::Text( "Enter new speaker name:" );
			ImGui::Spacing();
			ImGui::InputText( "##dropName", newSpeakerBuf, sizeof( newSpeakerBuf ) );
			if( showDropError )
			{
				ImGui::TextColored( ImVec4( 1.0f, 0.25f, 0.25f, 1.0f ), "%s", dropErrorBuf );
			}
			ImGui::Spacing();
			if( ImGui::Button( "OK" ) )
			{
				if( newSpeakerBuf[0] == '\0' )
				{
					idStr::Copynz( dropErrorBuf, "Name cannot be empty", sizeof( dropErrorBuf ) );
					showDropError = true;
				}
				else
				{
					const idDict* m	 = gameEdit->MapGetEntityDict( newSpeakerBuf );
					idEntity*	  ge = gameEdit->FindEntity( newSpeakerBuf );
					if( m || ge )
					{
						idStr::Copynz( dropErrorBuf, "Name already in use", sizeof( dropErrorBuf ) );
						showDropError = true;
					}
					else
					{
						bool ok = DropSpeaker( newSpeakerBuf );
						if( ok )
						{
							newSpeakerBuf[0] = '\0';
							showDropError	 = false;
							ImGui::CloseCurrentPopup();
						}
						else
						{
							idStr::Copynz( dropErrorBuf, "Failed to spawn speaker (see logs)", sizeof( dropErrorBuf ) );
							showDropError = true;
						}
					}
				}
			}
			ImGui::SameLine();
			if( ImGui::Button( "Cancel" ) )
			{
				showDropError = false;
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		ImGui::SeparatorText( "Sound Selection" );

		ImGui::Columns( 2, "sndcols", true );

		// Left: Sound Shaders
		ImGui::Text( "Sound Shaders" );
		ImGui::BeginChild( "##shaders", ImVec2( 0, 220 ), true );
		{
			idList<idStr>		  groupNames;
			idList<idList<idStr>> groupItems;
			groupNames.Clear();
			groupItems.Clear();
			for( int i = 0; i < soundShaders.Num(); i++ )
			{
				idStr full	   = soundShaders[i];
				int	  idx	   = full.Last( '/' );
				idStr filePart = ( idx >= 0 ) ? full.Left( idx ) : "<root>";
				int	  g		   = groupNames.FindIndex( filePart );
				if( g == -1 )
				{
					groupNames.Append( filePart );
					idList<idStr> items;
					items.Clear();
					items.Append( full );
					groupItems.Append( items );
				}
				else
				{
					groupItems[g].Append( full );
				}
			}

			for( int gi = 0; gi < groupNames.Num(); gi++ )
			{
				if( gi != 0 )
				{
					ImGui::Separator();
				}
				const idStr& grp = groupNames[gi];
				if( ImGui::TreeNode( grp.c_str() ) )
				{
					idList<idStr>& items = groupItems[gi];
					for( int k = 0; k < items.Num(); k++ )
					{
						const char* nm	= items[k].c_str();
						bool		sel = ( strShader.Length() && strShader.Icmp( nm ) == 0 );
						if( ImGui::Selectable( nm, sel ) )
						{
							strShader = nm;
							playSound = strShader;
							idStr::Copynz( shaderBuf, strShader.c_str(), sizeof( shaderBuf ) );

							// try to populate properties from the sound shader decl
							const idSoundShader* sd = declManager->FindSound( strShader, false );
							if( sd )
							{
								const soundShaderParms_t* p = sd->GetParms();
								if( p )
								{
									fVolume	   = p->volume;
									fMin	   = p->minDistance;
									fMax	   = p->maxDistance;
									shakes	   = p->shakes;
									int flags  = p->soundShaderFlags;
									bOmni	   = ( flags & SSF_OMNIDIRECTIONAL ) != 0;
									looping	   = ( flags & SSF_LOOPING ) != 0;
									unclamped  = ( flags & SSF_UNCLAMPED ) != 0;
									bOcclusion = ( flags & SSF_NO_OCCLUSION ) == 0;
								}
							}

							// Play on select if enabled
							if( bPlay )
							{
								idSoundWorld* sw = soundSystem->GetPlayingSoundWorld();
								if( sw )
								{
									sw->PlayShaderDirectly( playSound );
								}
							}

							// Double-click should play immediately
							// TODO: Why does not work?
							if( ImGui::IsMouseDoubleClicked( 0 ) && ImGui::IsItemHovered() )
							{
								const idStr	  toPlay = ( playSound.Length() ? playSound : strShader );
								idSoundWorld* sw	 = soundSystem->GetPlayingSoundWorld();
								if( sw )
								{
									sw->PlayShaderDirectly( toPlay );
								}
								ApplyChanges( false );
							}
						}
					}
					ImGui::TreePop();
				}
			}
		}
		ImGui::EndChild();

		ImGui::NextColumn();

		ImGui::Text( "Wave Files (sound/*)" );
		ImGui::BeginChild( "##waves", ImVec2( 0, 220 ), true );
		{
			for( int i = 0; i < soundFiles.Num(); i++ )
			{
				idStr		f	= soundFiles[i];
				const char* nm	= f.c_str();
				bool		sel = ( playSound.Length() && playSound.Icmp( nm ) == 0 );
				ImGui::PushID( i );
				if( ImGui::Selectable( nm, sel ) )
				{
					playSound = nm;
					idStr::Copynz( shaderBuf, playSound.c_str(), sizeof( shaderBuf ) );

					// if the selected wave corresponds to a sound shader, populate parms
					const idSoundShader* sd = declManager->FindSound( playSound, false );
					if( sd )
					{
						const soundShaderParms_t* p = sd->GetParms();
						if( p )
						{
							fVolume	   = p->volume;
							fMin	   = p->minDistance;
							fMax	   = p->maxDistance;
							shakes	   = p->shakes;
							int flags  = p->soundShaderFlags;
							bOmni	   = ( flags & SSF_OMNIDIRECTIONAL ) != 0;
							looping	   = ( flags & SSF_LOOPING ) != 0;
							unclamped  = ( flags & SSF_UNCLAMPED ) != 0;
							bOcclusion = ( flags & SSF_NO_OCCLUSION ) == 0;
						}
					}

					// Play on select if enabled
					if( bPlay )
					{
						idSoundWorld* sw = soundSystem->GetPlayingSoundWorld();
						if( sw )
						{
							sw->PlayShaderDirectly( playSound );
						}
					}

					// Double-click should play immediately
					// TODO: Why does not work?
					if( ImGui::IsMouseDoubleClicked( 0 ) && ImGui::IsItemHovered() )
					{
						const idStr	  toPlay = ( playSound.Length() ? playSound : strShader );
						idSoundWorld* sw	 = soundSystem->GetPlayingSoundWorld();
						if( sw )
						{
							sw->PlayShaderDirectly( toPlay );
						}
						ApplyChanges( false );
					}
				}
				ImGui::PopID();
			}
		}
		ImGui::EndChild();

		ImGui::Columns( 1 );

		ImGui::SeparatorText( "Properties" );
		if( ImGui::InputText( "Shader", shaderBuf, sizeof( shaderBuf ) ) )
		{
			strShader = shaderBuf;
		}
		if( ImGui::DragFloat( "Volume", &fVolume, 0.1f, -1000.0f, 1000.0f, "%.2f" ) )
		{
			idStr::snPrintf( volumeBuf, sizeof( volumeBuf ), "%.2f", fVolume );
		}
		ImGui::DragFloat( "Min Distance", &fMin, 0.1f, 0.0f, 10000.0f );
		ImGui::DragFloat( "Max Distance", &fMax, 0.1f, 0.0f, 10000.0f );

		ImGui::Spacing();

		ImGui::SeparatorText( "Speakers & Groups" );

		// Groups combo (only show if groups exist)
		groupsList.Clear();
		groupsList.SetNum( 1024 );
		int gcount = gameEdit->MapGetUniqueMatchingKeyVals( "soundgroup", groupsList.Ptr(), groupsList.Num() );
		groupsList.SetNum( gcount );
		if( gcount > 0 )
		{
			ImGui::Text( "Group" );
			if( selectedGroupIndex >= gcount )
			{
				selectedGroupIndex = -1;
			}
			if( selectedGroupIndex == -1 && strGroup.Length() )
			{
				for( int i = 0; i < gcount; i++ )
				{
					if( idStr::Icmp( groupsList[i], strGroup ) == 0 )
					{
						selectedGroupIndex = i;
						break;
					}
				}
			}
			if( ImGui::BeginCombo( "##Group", ( selectedGroupIndex >= 0 ) ? groupsList[selectedGroupIndex] : "<none>" ) )
			{
				for( int i = 0; i < gcount; i++ )
				{
					bool isSel = ( selectedGroupIndex == i );
					if( ImGui::Selectable( groupsList[i], isSel ) )
					{
						selectedGroupIndex = i;
						strGroup		   = groupsList[i];
						idStr::Copynz( groupBuf, strGroup.c_str(), sizeof( groupBuf ) );
					}
				}
				ImGui::EndCombo();
			}
		}

		// Speaker list
		speakersList.Clear();
		speakersList.SetNum( 512 );
		int scount = gameEdit->MapGetEntitiesMatchingClassWithString( "speaker", "", speakersList.Ptr(), speakersList.Num() );
		speakersList.SetNum( scount );
		if( scount > 0 )
		{
			ImGui::Text( "Speaker" );
			if( selectedSpeakerIndex >= scount )
			{
				selectedSpeakerIndex = -1;
			}
			if( ImGui::BeginCombo( "##Speaker", ( selectedSpeakerIndex >= 0 ) ? speakersList[selectedSpeakerIndex] : "<none>" ) )
			{
				for( int i = 0; i < scount; i++ )
				{
					bool isSel = ( selectedSpeakerIndex == i );
					if( ImGui::Selectable( speakersList[i], isSel ) )
					{
						selectedSpeakerIndex = i;
						// select in editor and auto-populate from speaker's sound shader
						gameEdit->ClearEntitySelection();
						idEntity* ge = gameEdit->FindEntity( speakersList[i] );
						if( ge )
						{
							gameEdit->AddSelectedEntity( ge );
							const idDict* d = gameEdit->EntityGetSpawnArgs( ge );
							Set( d );
							// extract shader name from speaker and auto-select it
							const char* shaderName = d->GetString( "s_shader" );
							if( shaderName && shaderName[0] )
							{
								strShader = shaderName;
								playSound = strShader;
								idStr::Copynz( shaderBuf, strShader.c_str(), sizeof( shaderBuf ) );
								// populate properties from shader decl if available
								const idSoundShader* sd = declManager->FindSound( shaderName, false );
								if( sd )
								{
									const soundShaderParms_t* p = sd->GetParms();
									if( p )
									{
										fVolume	   = p->volume;
										fMin	   = p->minDistance;
										fMax	   = p->maxDistance;
										shakes	   = p->shakes;
										int flags  = p->soundShaderFlags;
										bOmni	   = ( flags & SSF_OMNIDIRECTIONAL ) != 0;
										looping	   = ( flags & SSF_LOOPING ) != 0;
										unclamped  = ( flags & SSF_UNCLAMPED ) != 0;
										bOcclusion = ( flags & SSF_NO_OCCLUSION ) == 0;
									}
								}
							}
						}
					}
				}
				ImGui::EndCombo();
			}
		}

		// Show all shader flags (read-only) for the currently selected shader/wave
		const idSoundShader* curShaderDecl = NULL;
		if( strShader.Length() )
		{
			curShaderDecl = declManager->FindSound( strShader, false );
		}
		else if( playSound.Length() )
		{
			curShaderDecl = declManager->FindSound( playSound, false );
		}
		if( curShaderDecl )
		{
			const soundShaderParms_t* p = curShaderDecl->GetParms();
			if( p )
			{
				int	 flags		  = p->soundShaderFlags;
				bool fPrivate	  = ( flags & SSF_PRIVATE_SOUND ) != 0;
				bool fAntiPrivate = ( flags & SSF_ANTI_PRIVATE_SOUND ) != 0;
				bool fNoOcclusion = ( flags & SSF_NO_OCCLUSION ) != 0;
				bool fGlobal	  = ( flags & SSF_GLOBAL ) != 0;
				bool fOmni		  = ( flags & SSF_OMNIDIRECTIONAL ) != 0;
				bool fLoop		  = ( flags & SSF_LOOPING ) != 0;
				bool fPlayOnce	  = ( flags & SSF_PLAY_ONCE ) != 0;
				bool fUnclamped	  = ( flags & SSF_UNCLAMPED ) != 0;
				bool fNoFlicker	  = ( flags & SSF_NO_FLICKER ) != 0;
				bool fNoDups	  = ( flags & SSF_NO_DUPS ) != 0;
				ImGui::SeparatorText( "Shader Flags" );
				ImGui::BeginDisabled();
				if( ImGui::CollapsingHeader( "Advanced Flags", ImGuiTreeNodeFlags_DefaultOpen ) )
				{
					ImGui::Indent();
					ImGui::Checkbox( "Private Sound", &fPrivate );
					ImGui::Checkbox( "Anti-Private Sound", &fAntiPrivate );
					ImGui::Checkbox( "Global", &fGlobal );
					ImGui::Checkbox( "No Flicker", &fNoFlicker );
					ImGui::Checkbox( "No Duplicates", &fNoDups );
					ImGui::Unindent();
				}
				if( ImGui::CollapsingHeader( "Playback Flags", ImGuiTreeNodeFlags_DefaultOpen ) )
				{
					ImGui::Indent();
					ImGui::Checkbox( "Looping", &fLoop );
					ImGui::Checkbox( "Play Once", &fPlayOnce );
					ImGui::Checkbox( "Omnidirectional", &fOmni );
					ImGui::Checkbox( "Unclamped", &fUnclamped );
					ImGui::Unindent();
				}
				if( ImGui::CollapsingHeader( "Propagation Flags", ImGuiTreeNodeFlags_DefaultOpen ) )
				{
					ImGui::Indent();
					ImGui::Checkbox( "No Occlusion", &fNoOcclusion );
					ImGui::Unindent();
				}
				ImGui::EndDisabled();
			}
		}

		ImGui::End();
	}

	if( isShown && !showTool )
	{
		isShown = showTool;
		Exit();
	}
}

void SoundEditor::Exit( void )
{
	// close this tool, release mouse, clear editor flags and reset edit mode
	showTool = false;
	SoundEditor::Instance().ShowIt( false );
	impl::SetReleaseToolMouse( false );
	D3::ImGuiHooks::CloseWindow( D3::ImGuiHooks::D3_ImGuiWin_SoundEditor );
	// clear editor flag and reset edit mode cvar
	com_editors &= ~EDITOR_SOUND;
	cvarSystem->SetCVarInteger( "g_editEntityMode", 0 );
}

} // namespace ImGuiTools
