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

#ifndef __EDIT_PUBLIC_H__
#define __EDIT_PUBLIC_H__

/*
===============================================================================

	Editors.

===============================================================================
*/


class	idProgram;
class	idInterpreter;


// Radiant Level Editor
void	RadiantInit();
void	RadiantShutdown();
void	RadiantRun();
void	RadiantPrint( const char* text );


// in-game Light Editor
void	LightEditorInit( const idDict* spawnArgs );
void	LightEditorShutdown();
void	LightEditorRun();


// in-game Sound Editor
void	SoundEditorInit( const idDict* spawnArgs );
void	SoundEditorShutdown();
void	SoundEditorRun();


// in-game Articulated Figure Editor
void	AFEditorInit( const idDict* spawnArgs );
void	AFEditorShutdown();
void	AFEditorRun();


// in-game Particle Editor
void	ParticleEditorInit( const idDict* spawnArgs );
void	ParticleEditorShutdown();
void	ParticleEditorRun();

// in-game Script Editor
void	ScriptEditorInit( const idDict* spawnArgs );
void	ScriptEditorShutdown();
void	ScriptEditorRun();


// in-game Declaration Browser
void	DeclBrowserInit( const idDict* spawnArgs );
void	DeclBrowserShutdown();
void	DeclBrowserRun();
void	DeclBrowserReloadDeclarations();


// GUI Editor
void	GUIEditorInit();
void	GUIEditorShutdown();
void	GUIEditorRun();
bool	GUIEditorHandleMessage( void* msg );


// Script Debugger
void	DebuggerClientLaunch();
void	DebuggerClientInit( const char* cmdline );
bool	DebuggerServerInit();
void	DebuggerServerShutdown();
void	DebuggerServerPrint( const char* text );
void	DebuggerServerCheckBreakpoint( idInterpreter* interpreter, idProgram* program, int instructionPointer );

//Material Editor
void	MaterialEditorInit();
void	MaterialEditorRun();
void	MaterialEditorShutdown();
void	MaterialEditorPrintConsole( const char* msg );

#endif /* !__EDIT_PUBLIC_H__ */
