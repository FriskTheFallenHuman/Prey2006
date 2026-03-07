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

#include "Common_local.h"

int				com_editors;			// currently opened editor(s)
bool			com_editorActive;		//  true if an editor has focus

#ifdef _WIN32
HWND			com_hwndMsg = NULL;
bool			com_outputMsg = false;
unsigned int	com_msgID = -1;
#endif

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
bool FindEditor() {
	com_hwndMsg = NULL;
	EnumWindows( EnumWindowsProc, 0 );
	return !( com_hwndMsg == NULL );
}

#endif

/*
=================
idCommonLocal::InitTool
=================
*/
void idCommonLocal::InitTool( const toolFlag_t tool, const idDict *dict ) {
#ifdef ID_ALLOW_TOOLS
	if ( tool & EDITOR_SOUND ) {
		SoundEditorInit( dict );
	} else if ( tool & EDITOR_LIGHT ) {
		LightEditorInit( dict );
	} else if ( tool & EDITOR_PARTICLE ) {
		ParticleEditorInit( dict );
	} else if ( tool & EDITOR_AF ) {
		AFEditorInit( dict );
	}
#endif
}

/*
==================
idCommonLocal::ActivateTool

Activates or Deactivates a tool
==================
*/
void idCommonLocal::ActivateTool( bool active ) {
	com_editorActive = active;
}

/*
==================
idCommonLocal::CheckToolMode

Check for "renderbump", "dmap", or "editor" on the command line,
and force fullscreen off in those cases
==================
*/
void idCommonLocal::CheckToolMode() {
	int			i;

	for ( i = 0 ; i < com_numConsoleLines ; i++ ) {
		if ( !idStr::Icmp( com_consoleLines[ i ].Argv(0), "guieditor" ) ) {
			com_editors |= EDITOR_GUI;
		}
		else if ( !idStr::Icmp( com_consoleLines[ i ].Argv(0), "debugger" ) ) {
			com_editors |= EDITOR_DEBUGGER;
		}
		else if ( !idStr::Icmp( com_consoleLines[ i ].Argv(0), "editor" ) ) {
			com_editors |= EDITOR_RADIANT;
		}
		// Nerve: Add support for the material editor
		else if ( !idStr::Icmp( com_consoleLines[ i ].Argv(0), "materialEditor" ) ) {
			com_editors |= EDITOR_MATERIAL;
		}

		if ( !idStr::Icmp( com_consoleLines[ i ].Argv(0), "renderbump" )
			|| !idStr::Icmp( com_consoleLines[ i ].Argv(0), "editor" )
			|| !idStr::Icmp( com_consoleLines[ i ].Argv(0), "guieditor" )
			|| !idStr::Icmp( com_consoleLines[ i ].Argv(0), "debugger" )
			|| !idStr::Icmp( com_consoleLines[ i ].Argv(0), "dmap" )
			|| !idStr::Icmp( com_consoleLines[ i ].Argv(0), "materialEditor" )
			) {
			cvarSystem->SetCVarBool( "r_fullscreen", false );
			return;
		}
	}
}

/*
=================
idCommonLocal::InitCommands
=================
*/
void idCommonLocal::InitCommands( void ) {
#if	!defined( ID_DEDICATED )
	// compilers
	cmdSystem->AddCommand( "dmap", Dmap_f, CMD_FL_TOOL, "compiles a map", idCmdSystem::ArgCompletion_MapName );
	cmdSystem->AddCommand( "renderbump", RenderBump_f, CMD_FL_TOOL, "renders a bump map", idCmdSystem::ArgCompletion_ModelName );
	cmdSystem->AddCommand( "renderbumpFlat", RenderBumpFlat_f, CMD_FL_TOOL, "renders a flat bump map", idCmdSystem::ArgCompletion_ModelName );
	cmdSystem->AddCommand( "runAAS", RunAAS_f, CMD_FL_TOOL, "compiles an AAS file for a map", idCmdSystem::ArgCompletion_MapName );
	cmdSystem->AddCommand( "runAASDir", RunAASDir_f, CMD_FL_TOOL, "compiles AAS files for all maps in a folder", idCmdSystem::ArgCompletion_MapName );
	cmdSystem->AddCommand( "runReach", RunReach_f, CMD_FL_TOOL, "calculates reachability for an AAS file", idCmdSystem::ArgCompletion_MapName );
	cmdSystem->AddCommand( "roq", RoQFileEncode_f, CMD_FL_TOOL, "encodes a roq file" );
	cmdSystem->AddCommand( "amplitude", Amplitude_f, CMD_FL_TOOL, "encodes a wav file into a amp file", idCmdSystem::ArgCompletion_SoundName );
#endif

#ifdef ID_ALLOW_TOOLS
	// editors
	cmdSystem->AddCommand( "editor", Com_Editor_f, CMD_FL_TOOL, "launches the level editor Radiant" );
	cmdSystem->AddCommand( "editLights", Com_EditLights_f, CMD_FL_TOOL, "launches the in-game Light Editor" );
	cmdSystem->AddCommand( "editSounds", Com_EditSounds_f, CMD_FL_TOOL, "launches the in-game Sound Editor" );
	cmdSystem->AddCommand( "editDecls", Com_EditDecls_f, CMD_FL_TOOL, "launches the in-game Declaration Editor" );
	cmdSystem->AddCommand( "editAFs", Com_EditAFs_f, CMD_FL_TOOL, "launches the in-game Articulated Figure Editor" );
	cmdSystem->AddCommand( "editParticles", Com_EditParticles_f, CMD_FL_TOOL, "launches the in-game Particle Editor" );
	cmdSystem->AddCommand( "editScripts", Com_EditScripts_f, CMD_FL_TOOL, "launches the in-game Script Editor" );
	cmdSystem->AddCommand( "editGUIs", Com_EditGUIs_f, CMD_FL_TOOL, "launches the GUI Editor" );
	cmdSystem->AddCommand( "editPDAs", Com_EditPDAs_f, CMD_FL_TOOL, "launches the in-game PDA Editor" );
	cmdSystem->AddCommand( "debugger", Com_ScriptDebugger_f, CMD_FL_TOOL, "launches the Script Debugger" );

	//BSM Nerve: Add support for the material editor
	cmdSystem->AddCommand( "materialEditor", Com_MaterialEditor_f, CMD_FL_TOOL, "launches the Material Editor" );
#endif
}