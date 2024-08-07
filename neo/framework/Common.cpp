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

#include "ConsoleHistory.h"

#include "../renderer/Image.h"

#include "Session_local.h" // DG: For FT_IsDemo/isDemo() hack

#if defined( _DEBUG )
	#define BUILD_DEBUG "-debug"
#else
	#define BUILD_DEBUG ""
#endif

struct version_s {
			version_s( void ) { sprintf( string, "%s.%d%s %s-%s %s %s", ENGINE_VERSION, BUILD_NUMBER, BUILD_DEBUG, BUILD_OS, BUILD_CPU, ID__DATE__, ID__TIME__ ); }
	char	string[256];
} version;

idCVar com_version( "si_version", version.string, CVAR_SYSTEM|CVAR_ROM|CVAR_SERVERINFO, "engine version" );
idCVar com_forceGenericSIMD( "com_forceGenericSIMD", "0", CVAR_BOOL | CVAR_SYSTEM | CVAR_NOCHEAT, "force generic platform independent SIMD" );

idCVar com_allowConsole( "com_allowConsole", "0", CVAR_BOOL | CVAR_SYSTEM | CVAR_NOCHEAT, "allow toggling console with the tilde key" );

idCVar com_developer( "developer", "0", CVAR_BOOL|CVAR_SYSTEM|CVAR_NOCHEAT, "developer mode" );
idCVar com_speeds( "com_speeds", "0", CVAR_BOOL|CVAR_SYSTEM|CVAR_NOCHEAT, "show engine timings" );
idCVar com_showFPS( "com_showFPS", "0", CVAR_BOOL|CVAR_SYSTEM|CVAR_ARCHIVE|CVAR_NOCHEAT, "show frames rendered per second" );
idCVar com_showMemoryUsage( "com_showMemoryUsage", "0", CVAR_BOOL|CVAR_SYSTEM|CVAR_NOCHEAT, "show total and per frame memory usage" );
idCVar com_showAsyncStats( "com_showAsyncStats", "0", CVAR_BOOL|CVAR_SYSTEM|CVAR_NOCHEAT, "show async network stats" );
idCVar com_showSoundDecoders( "com_showSoundDecoders", "0", CVAR_BOOL|CVAR_SYSTEM|CVAR_NOCHEAT, "show sound decoders" );

idCVar com_skipRenderer( "com_skipRenderer", "0", CVAR_BOOL|CVAR_SYSTEM, "skip the renderer completely" );
idCVar com_imageQuality( "com_imageQuality", "-1", CVAR_INTEGER | CVAR_ARCHIVE | CVAR_SYSTEM, "hardware classification, -1 = not detected, 0 = low quality, 1 = medium quality, 2 = high quality, 3 = ultra quality" );
idCVar com_purgeAll( "com_purgeAll", "0", CVAR_BOOL | CVAR_ARCHIVE | CVAR_SYSTEM, "purge everything between level loads" );
idCVar com_memoryMarker( "com_memoryMarker", "-1", CVAR_INTEGER | CVAR_SYSTEM | CVAR_INIT, "used as a marker for memory stats" );
idCVar com_preciseTic( "com_preciseTic", "1", CVAR_BOOL|CVAR_SYSTEM, "run one game tick every async thread update" );
idCVar com_asyncInput( "com_asyncInput", "0", CVAR_BOOL|CVAR_SYSTEM, "sample input from the async thread" );
#define ASYNCSOUND_INFO "0: mix sound inline, 1 or 3: async update every 16ms 2: async update about every 100ms (original behavior)"
idCVar com_asyncSound( "com_asyncSound", "1", CVAR_INTEGER|CVAR_SYSTEM, ASYNCSOUND_INFO, 0, 3 );

idCVar com_makingBuild( "com_makingBuild", "0", CVAR_BOOL | CVAR_SYSTEM, "1 when making a build" );

idCVar com_enableDebuggerServer( "com_enableDebuggerServer", "0", CVAR_BOOL | CVAR_SYSTEM, "toggle debugger server and try to connect to com_dbgClientAdr" );
idCVar com_dbgClientAdr( "com_dbgClientAdr", "localhost", CVAR_SYSTEM | CVAR_ARCHIVE, "debuggerApp client address" );
idCVar com_dbgServerAdr( "com_dbgServerAdr", "localhost", CVAR_SYSTEM | CVAR_ARCHIVE, "debugger server address" );

// com_speeds times
int				time_gameFrame;
int				time_gameDraw;
int				time_frontend;			// renderSystem frontend time
int				time_backend;			// renderSystem backend time

int				com_frameTime;			// time for the current frame in milliseconds
int				com_frameNumber;		// variable frame number
volatile int	com_ticNumber;			// 60 hz tics
int				com_editors;			// currently opened editor(s)
bool			com_editorActive;		//  true if an editor has focus

bool			com_debuggerSupported;	// only set to true when the updateDebugger function is set. see GetAdditionalFunction()

#ifdef _WIN32
HWND			com_hwndMsg = NULL;
bool			com_outputMsg = false;
#endif

#ifdef __DOOM_DLL__
idGame *		game = NULL;
idGameEdit *	gameEdit = NULL;
#endif

// writes si_version to the config file - in a kinda obfuscated way
//#define ID_WRITE_VERSION

idCommonLocal	commonLocal;
idCommon *		common = &commonLocal;

/*
==================
idCommonLocal::idCommonLocal
==================
*/
idCommonLocal::idCommonLocal( void ) {
	com_fullyInitialized = false;
	com_refreshOnPrint = false;
	com_errorEntered = 0;
	com_debuggerSupported = false;

	strcpy( errorMessage, "" );

	rd_buffer = NULL;
	rd_buffersize = 0;
	rd_flush = NULL;

	gameDLL = 0;

#ifdef ID_WRITE_VERSION
	config_compressor = NULL;
#endif

	async_timer = 0;
}

/*
==================
idCommonLocal::Quit
==================
*/
void idCommonLocal::Quit( void ) {

#ifdef ID_ALLOW_TOOLS
	if ( com_editors & EDITOR_RADIANT ) {
		RadiantInit();
		return;
	}
#endif

	// don't try to shutdown if we are in a recursive error
	if ( !com_errorEntered ) {
		Shutdown();
	}

	Sys_Quit();
}


/*
============================================================================

COMMAND LINE FUNCTIONS

+ characters separate the commandLine string into multiple console
command lines.

All of these are valid:

doom +set test blah +map test
doom set test blah+map test
doom set test blah + map test

============================================================================
*/

#define		MAX_CONSOLE_LINES	32
int			com_numConsoleLines;
idCmdArgs	com_consoleLines[MAX_CONSOLE_LINES];

/*
==================
idCommonLocal::ParseCommandLine
==================
*/
void idCommonLocal::ParseCommandLine( int argc, char **argv ) {
	int i;

	com_numConsoleLines = 0;
	// API says no program path
	for ( i = 0; i < argc; i++ ) {
		if ( argv[ i ][ 0 ] == '+' ) {
			com_numConsoleLines++;
			com_consoleLines[ com_numConsoleLines-1 ].AppendArg( argv[ i ] + 1 );
		} else {
			if ( !com_numConsoleLines ) {
				com_numConsoleLines++;
			}
			com_consoleLines[ com_numConsoleLines-1 ].AppendArg( argv[ i ] );
		}
	}
}

/*
==================
idCommonLocal::ClearCommandLine
==================
*/
void idCommonLocal::ClearCommandLine( void ) {
	com_numConsoleLines = 0;
}

/*
==================
idCommonLocal::SafeMode

Check for "safe" on the command line, which will
skip loading of config file (DoomConfig.cfg)
==================
*/
bool idCommonLocal::SafeMode( void ) {
	int			i;

	for ( i = 0 ; i < com_numConsoleLines ; i++ ) {
		if ( !idStr::Icmp( com_consoleLines[ i ].Argv(0), "safe" )
			|| !idStr::Icmp( com_consoleLines[ i ].Argv(0), "cvar_restart" ) ) {
			com_consoleLines[ i ].Clear();
			return true;
		}
	}
	return false;
}

/*
==================
idCommonLocal::CheckToolMode

Check for "renderbump", "dmap", or "editor" on the command line,
and force fullscreen off in those cases
==================
*/
void idCommonLocal::CheckToolMode( void ) {
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
==================
idCommonLocal::StartupVariable

Searches for command line parameters that are set commands.
If match is not NULL, only that cvar will be looked for.
That is necessary because cddir and basedir need to be set
before the filesystem is started, but all other sets should
be after execing the config and default.
==================
*/
void idCommonLocal::StartupVariable( const char *match, bool once ) {
	int			i;
	const char *s;

	i = 0;
	while (	i < com_numConsoleLines ) {
		if ( strcmp( com_consoleLines[ i ].Argv( 0 ), "set" ) ) {
			i++;
			continue;
		}

		s = com_consoleLines[ i ].Argv(1);

		if ( !match || !idStr::Icmp( s, match ) ) {
			cvarSystem->SetCVarString( s, com_consoleLines[ i ].Argv( 2 ) );
			if ( once ) {
				// kill the line
				int j = i + 1;
				while ( j < com_numConsoleLines ) {
					com_consoleLines[ j - 1 ] = com_consoleLines[ j ];
					j++;
				}
				com_numConsoleLines--;
				continue;
			}
		}
		i++;
	}
}

/*
==================
idCommonLocal::AddStartupCommands

Adds command line parameters as script statements
Commands are separated by + signs

Returns true if any late commands were added, which
will keep the demoloop from immediately starting
==================
*/
bool idCommonLocal::AddStartupCommands( void ) {
	int		i;
	bool	added;

	added = false;
	// quote every token, so args with semicolons can work
	for ( i = 0; i < com_numConsoleLines; i++ ) {
		if ( !com_consoleLines[i].Argc() ) {
			continue;
		}

		// set commands won't override menu startup
		if ( idStr::Icmpn( com_consoleLines[i].Argv(0), "set", 3 ) ) {
			added = true;
		}
		// directly as tokenized so nothing gets screwed
		cmdSystem->BufferCommandArgs( CMD_EXEC_APPEND, com_consoleLines[i] );
	}

	return added;
}

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
idCommonLocal::WriteFlaggedCVarsToFile
==================
*/
void idCommonLocal::WriteFlaggedCVarsToFile( const char *filename, int flags, const char *setCmd ) {
	idFile *f;

	f = fileSystem->OpenFileWrite( filename, "fs_configpath" );
	if ( !f ) {
		Printf( "Couldn't write %s.\n", filename );
		return;
	}
	cvarSystem->WriteFlaggedVariables( flags, setCmd, f );
	fileSystem->CloseFile( f );
}

/*
==================
idCommonLocal::WriteConfigToFile
==================
*/
void idCommonLocal::WriteConfigToFile( const char *filename ) {
	idFile *f;
#ifdef ID_WRITE_VERSION
	ID_TIME_T t;
	char *curtime;
	idStr runtag;
	idFile_Memory compressed( "compressed" );
	idBase64 out;
#endif

	f = fileSystem->OpenFileWrite( filename, "fs_configpath" );
	if ( !f ) {
		Printf ("Couldn't write %s.\n", filename );
		return;
	}

#ifdef ID_WRITE_VERSION
	assert( config_compressor );
	t = time( NULL );
	curtime = ctime( &t );
	sprintf( runtag, "%s - %s", cvarSystem->GetCVarString( "si_version" ), curtime );
	config_compressor->Init( &compressed, true, 8 );
	config_compressor->Write( runtag.c_str(), runtag.Length() );
	config_compressor->FinishCompress( );
	out.Encode( (const byte *)compressed.GetDataPtr(), compressed.Length() );
	f->Printf( "// %s\n", out.c_str() );
#endif

	idKeyInput::WriteBindings( f );
	cvarSystem->WriteFlaggedVariables( CVAR_ARCHIVE, "seta", f );
	fileSystem->CloseFile( f );
}

/*
===============
idCommonLocal::WriteConfiguration

Writes key bindings and archived cvars to config file if modified
===============
*/
void idCommonLocal::WriteConfiguration( void ) {
	// if we are quiting without fully initializing, make sure
	// we don't write out anything
	if ( !com_fullyInitialized ) {
		return;
	}

	if ( !( cvarSystem->GetModifiedFlags() & CVAR_ARCHIVE ) ) {
		return;
	}
	cvarSystem->ClearModifiedFlags( CVAR_ARCHIVE );

	// disable printing out the "Writing to:" message
	bool developer = com_developer.GetBool();
	com_developer.SetBool( false );

	WriteConfigToFile( CONFIG_FILE );
	session->WriteCDKey( );

	// restore the developer cvar
	com_developer.SetBool( developer );
}

/*
===============
KeysFromBinding()
Returns the key bound to the command
===============
*/
const char* idCommonLocal::KeysFromBinding( const char *bind ) {
	return idKeyInput::KeysFromBinding( bind );
}

/*
===============
BindingFromKey()
Returns the binding bound to key
===============
*/
const char* idCommonLocal::BindingFromKey( const char *key ) {
	return idKeyInput::BindingFromKey( key );
}

/*
===============
ButtonState()
Returns the state of the button
===============
*/
int	idCommonLocal::ButtonState( int key ) {
	return usercmdGen->ButtonState(key);
}

/*
===============
ButtonState()
Returns the state of the key
===============
*/
int	idCommonLocal::KeyState( int key ) {
	return usercmdGen->KeyState(key);
}

//============================================================================

#ifdef ID_ALLOW_TOOLS
/*
==================
Com_Editor_f

  we can start the editor dynamically, but we won't ever get back
==================
*/
static void Com_Editor_f( const idCmdArgs &args ) {
	RadiantInit();
}

/*
=============
Com_ScriptDebugger_f
=============
*/
static void Com_ScriptDebugger_f( const idCmdArgs &args ) {
	// Make sure it wasnt on the command line
	if ( !( com_editors & EDITOR_DEBUGGER ) ) {
		
		//start debugger server if needed
		if ( !com_enableDebuggerServer.GetBool() )
			com_enableDebuggerServer.SetBool( true );

		//start debugger client.
		DebuggerClientLaunch();

	}
}

/*
=============
Com_EditGUIs_f
=============
*/
static void Com_EditGUIs_f( const idCmdArgs &args ) {
	GUIEditorInit();
}

/*
=============
Com_MaterialEditor_f
=============
*/
static void Com_MaterialEditor_f( const idCmdArgs &args ) {
	// Turn off sounds
	soundSystem->SetMute( true );
	MaterialEditorInit();
}
#endif // ID_ALLOW_TOOLS

/*
============
idCmdSystemLocal::PrintMemInfo_f

This prints out memory debugging data
============
*/
CONSOLE_COMMAND( printMemInfo, "prints memory debugging data", NULL ) {
	MemInfo_t mi;

	memset( &mi, 0, sizeof( mi ) );
	mi.filebase = session->GetCurrentMapName();

	renderSystem->PrintMemInfo( &mi );			// textures and models
	soundSystem->PrintMemInfo( &mi );			// sounds

	common->Printf( " Used image memory: %s bytes\n", idStr::FormatNumber( mi.imageAssetsTotal ).c_str() );
	mi.assetTotals += mi.imageAssetsTotal;

	common->Printf( " Used model memory: %s bytes\n", idStr::FormatNumber( mi.modelAssetsTotal ).c_str() );
	mi.assetTotals += mi.modelAssetsTotal;

	common->Printf( " Used sound memory: %s bytes\n", idStr::FormatNumber( mi.soundAssetsTotal ).c_str() );
	mi.assetTotals += mi.soundAssetsTotal;

	common->Printf( " Used asset memory: %s bytes\n", idStr::FormatNumber( mi.assetTotals ).c_str() );

	// write overview file
	idFile *f;

	f = fileSystem->OpenFileAppend( "maps/printmeminfo.txt" );
	if ( !f ) {
		return;
	}

	f->Printf( "total(%s ) image(%s ) model(%s ) sound(%s ): %s\n", idStr::FormatNumber( mi.assetTotals ).c_str(), idStr::FormatNumber( mi.imageAssetsTotal ).c_str(),
		idStr::FormatNumber( mi.modelAssetsTotal ).c_str(), idStr::FormatNumber( mi.soundAssetsTotal ).c_str(), mi.filebase.c_str() );

	fileSystem->CloseFile( f );
}

#ifdef ID_ALLOW_TOOLS
/*
==================
Com_EditLights_f
==================
*/
static void Com_EditLights_f( const idCmdArgs &args ) {
	LightEditorInit( NULL );
	cvarSystem->SetCVarInteger( "g_editEntityMode", 1 );
}

/*
==================
Com_EditSounds_f
==================
*/
static void Com_EditSounds_f( const idCmdArgs &args ) {
	SoundEditorInit( NULL );
	cvarSystem->SetCVarInteger( "g_editEntityMode", 2 );
}

/*
==================
Com_EditDecls_f
==================
*/
static void Com_EditDecls_f( const idCmdArgs &args ) {
	DeclBrowserInit( NULL );
}

/*
==================
Com_EditAFs_f
==================
*/
static void Com_EditAFs_f( const idCmdArgs &args ) {
	AFEditorInit( NULL );
}

/*
==================
Com_EditParticles_f
==================
*/
static void Com_EditParticles_f( const idCmdArgs &args ) {
	ParticleEditorInit( NULL );
}

/*
==================
Com_EditScripts_f
==================
*/
static void Com_EditScripts_f( const idCmdArgs &args ) {
	ScriptEditorInit( NULL );
}
#endif // ID_ALLOW_TOOLS

/*
==================
Com_Error_f

Just throw a fatal error to test error shutdown procedures.
==================
*/
CONSOLE_COMMAND( error, "causes an error", NULL ) {
	if ( !com_developer.GetBool() ) {
		commonLocal.Printf( "error may only be used in developer mode\n" );
		return;
	}

	if ( args.Argc() > 1 ) {
		commonLocal.FatalError( "Testing fatal error" );
	} else {
		commonLocal.Error( "Testing drop error" );
	}
}

/*
==================
Com_Freeze_f

Just freeze in place for a given number of seconds to test error recovery.
==================
*/
CONSOLE_COMMAND( freeze, "freezes the game for a number of seconds", NULL ) {
	float	s;
	int		start, now;

	if ( args.Argc() != 2 ) {
		commonLocal.Printf( "freeze <seconds>\n" );
		return;
	}

	if ( !com_developer.GetBool() ) {
		commonLocal.Printf( "freeze may only be used in developer mode\n" );
		return;
	}

	s = atof( args.Argv(1) );

	start = eventLoop->Milliseconds();

	while ( 1 ) {
		now = eventLoop->Milliseconds();
		if ( ( now - start ) * 0.001f > s ) {
			break;
		}
	}
}

/*
=================
Com_Crash_f

A way to force a bus error for development reasons
=================
*/
CONSOLE_COMMAND( crash, "causes a crash", NULL ) {
	if ( !com_developer.GetBool() ) {
		commonLocal.Printf( "crash may only be used in developer mode\n" );
		return;
	}

#ifdef __GNUC__
	__builtin_trap();
#else
	* ( int * ) 0 = 0x12345678;
#endif
}

/*
=================
Com_Quit_f
=================
*/
CONSOLE_COMMAND_SHIP( quit, "quits the game", NULL ) {
	commonLocal.Quit();
}
CONSOLE_COMMAND_SHIP( exit, "exits the game", NULL ) {
	commonLocal.Quit();
}

/*
===============
Com_WriteConfig_f

Write the config file to a specific name
===============
*/
CONSOLE_COMMAND( writeConfig, "writes a config file", NULL ) {
	idStr	filename;

	if ( args.Argc() != 2 ) {
		commonLocal.Printf( "Usage: writeconfig <filename>\n" );
		return;
	}

	filename = args.Argv(1);
	filename.DefaultFileExtension( ".cfg" );
	commonLocal.Printf( "Writing %s.\n", filename.c_str() );
	commonLocal.WriteConfigToFile( filename );
}

/*
=================
Com_SetMachineSpecs_f
=================
*/
CONSOLE_COMMAND( setMachineSpec, "detects system capabilities and sets com_machineSpec to appropriate value", NULL ) {
	commonLocal.SetMachineSpec();
}

/*
=================
Com_ExecMachineSpecs_f
=================
*/
CONSOLE_COMMAND( execMachineSpec, "execs the appropriate config files and sets cvars based on com_machineSpec", NULL ) {
	// DG: add an optional "nores" argument for "don't change the resolution" (r_mode)
	bool nores = args.Argc() > 1 && idStr::Icmp( args.Argv(1), "nores" ) == 0;
	if ( com_imageQuality.GetInteger() == 3 ) { // ultra
		//cvarSystem->SetCVarInteger( "image_anisotropy", 1, CVAR_ARCHIVE ); DG: redundant, set again below
		cvarSystem->SetCVarInteger( "image_lodbias", 0, CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "image_forceDownSize", 0, CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "image_roundDown", 1, CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "image_preload", 1, CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "image_useAllFormats", 1, CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "image_downSizeSpecular", 0, CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "image_downSizeBump", 0, CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "image_downSizeSpecularLimit", 64, CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "image_downSizeBumpLimit", 256, CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "image_usePrecompressedTextures", 0, CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "image_downsize", 0			, CVAR_ARCHIVE );
		cvarSystem->SetCVarString( "image_filter", "GL_LINEAR_MIPMAP_LINEAR", CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "image_anisotropy", 8, CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "image_useCompression", 0, CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "image_ignoreHighQuality", 0, CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "s_maxSoundsPerShader", 0, CVAR_ARCHIVE );
		if ( !nores ) // DG: added optional "nores" argument
			cvarSystem->SetCVarInteger( "r_mode", 5, CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "image_useNormalCompression", 0, CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "r_multiSamples", 0, CVAR_ARCHIVE );
	} else if ( com_imageQuality.GetInteger() == 2 ) { // high
		cvarSystem->SetCVarString( "image_filter", "GL_LINEAR_MIPMAP_LINEAR", CVAR_ARCHIVE );
		//cvarSystem->SetCVarInteger( "image_anisotropy", 1, CVAR_ARCHIVE ); DG: redundant, set again below
		cvarSystem->SetCVarInteger( "image_lodbias", 0, CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "image_forceDownSize", 0, CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "image_roundDown", 1, CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "image_preload", 1, CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "image_useAllFormats", 1, CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "image_downSizeSpecular", 0, CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "image_downSizeBump", 0, CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "image_downSizeSpecularLimit", 64, CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "image_downSizeBumpLimit", 256, CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "image_usePrecompressedTextures", 1, CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "image_downsize", 0, CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "image_anisotropy", 8, CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "image_useCompression", 1, CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "image_ignoreHighQuality", 0, CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "s_maxSoundsPerShader", 0, CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "image_useNormalCompression", 0, CVAR_ARCHIVE );
		if ( !nores ) // DG: added optional "nores" argument
			cvarSystem->SetCVarInteger( "", 4, CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "r_multiSamples", 0, CVAR_ARCHIVE );
	} else if ( com_imageQuality.GetInteger() == 1 ) { // medium
		cvarSystem->SetCVarString( "image_filter", "GL_LINEAR_MIPMAP_LINEAR", CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "image_anisotropy", 1, CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "image_lodbias", 0, CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "image_downSize", 0, CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "image_forceDownSize", 0, CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "image_roundDown", 1, CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "image_preload", 1, CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "image_useCompression", 1, CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "image_useAllFormats", 1, CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "image_usePrecompressedTextures", 1, CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "image_downSizeSpecular", 0, CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "image_downSizeBump", 0, CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "image_downSizeSpecularLimit", 64, CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "image_downSizeBumpLimit", 256, CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "image_useNormalCompression", 2, CVAR_ARCHIVE );
		if ( !nores ) // DG: added optional "nores" argument
			cvarSystem->SetCVarInteger( "r_mode", 3, CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "r_multiSamples", 0, CVAR_ARCHIVE );
	} else { // low
		cvarSystem->SetCVarString( "image_filter", "GL_LINEAR_MIPMAP_LINEAR", CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "image_anisotropy", 1, CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "image_lodbias", 0, CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "image_roundDown", 1, CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "image_preload", 1, CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "image_useAllFormats", 1, CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "image_usePrecompressedTextures", 1, CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "image_downSize", 1, CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "image_anisotropy", 0, CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "image_useCompression", 1, CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "image_ignoreHighQuality", 1, CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "s_maxSoundsPerShader", 1, CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "image_downSizeSpecular", 1, CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "image_downSizeBump", 1, CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "image_downSizeSpecularLimit", 64, CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "image_downSizeBumpLimit", 256, CVAR_ARCHIVE );
		if ( !nores ) // DG: added optional "nores" argument
			cvarSystem->SetCVarInteger( "r_mode", 3	, CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "image_useNormalCompression", 2, CVAR_ARCHIVE );
		cvarSystem->SetCVarInteger( "r_multiSamples", 0, CVAR_ARCHIVE );
	}

	cvarSystem->SetCVarBool( "com_purgeAll", false, CVAR_ARCHIVE );
	cvarSystem->SetCVarBool( "r_forceLoadImages", false, CVAR_ARCHIVE );

	cvarSystem->SetCVarBool( "g_decals", true, CVAR_ARCHIVE );
	cvarSystem->SetCVarBool( "g_projectileLights", true, CVAR_ARCHIVE );
	cvarSystem->SetCVarBool( "g_doubleVision", true, CVAR_ARCHIVE );
	cvarSystem->SetCVarBool( "g_muzzleFlash", true, CVAR_ARCHIVE );
}

/*
=================
Com_ReloadEngine_f
=================
*/
CONSOLE_COMMAND( reloadEngine, "reloads the engine down to including the file system", NULL ) {
	bool menu = false;

	if ( !commonLocal.IsInitialized() ) {
		return;
	}

	if ( args.Argc() > 1 && idStr::Icmp( args.Argv( 1 ), "menu" ) == 0 ) {
		menu = true;
	}

	common->Printf( "============= ReloadEngine start =============\n" );
	if ( !menu ) {
		Sys_ShowConsole( 1, false );
	}
	commonLocal.ShutdownGame( true );
	commonLocal.InitGame();
	if ( !menu && !idAsyncNetwork::serverDedicated.GetBool() ) {
		Sys_ShowConsole( 0, false );
	}
	common->Printf( "============= ReloadEngine end ===============\n" );

	if ( !cmdSystem->PostReloadEngine() ) {
		if ( menu ) {
			session->StartMenu( );
		}
	}
}

/*
===============
idCommonLocal::GetLanguageDict
===============
*/
const idLangDict *idCommonLocal::GetLanguageDict( void ) {
	return &languageDict;
}

/*
===============
idCommonLocal::FilterLangList
===============
*/
void idCommonLocal::FilterLangList( idStrList* list, idStr lang ) {

	idStr temp;
	for( int i = 0; i < list->Num(); i++ ) {
		temp = (*list)[i];
		temp = temp.Right(temp.Length()-strlen("strings/"));
		temp = temp.Left(lang.Length());
		if(idStr::Icmp(temp, lang) != 0) {
			list->RemoveIndex(i);
			i--;
		}
	}
}

/*
===============
idCommonLocal::InitLanguageDict
===============
*/
void idCommonLocal::InitLanguageDict( void ) {
	idStr fileName;
	languageDict.Clear();

	//D3XP: Instead of just loading a single lang file for each language
	//we are going to load all files that begin with the language name
	//similar to the way pak files work. So you can place english001.lang
	//to add new strings to the english language dictionary
	idFileList*	langFiles;
	langFiles =  fileSystem->ListFilesTree( "strings", ".lang", true );

	idStrList langList = langFiles->GetList();

	StartupVariable( "sys_lang", false );	// let it be set on the command line - this is needed because this init happens very early
	idStr langName = cvarSystem->GetCVarString( "sys_lang" );

	//Loop through the list and filter
	idStrList currentLangList = langList;
	FilterLangList(&currentLangList, langName);

	if ( currentLangList.Num() == 0 ) {
		// reset cvar to default and try to load again
		cmdSystem->BufferCommandText( CMD_EXEC_NOW, "reset sys_lang" );
		langName = cvarSystem->GetCVarString( "sys_lang" );
		currentLangList = langList;
		FilterLangList(&currentLangList, langName);
	}

	for( int i = 0; i < currentLangList.Num(); i++ ) {
		//common->Printf("%s\n", currentLangList[i].c_str());
		languageDict.Load( currentLangList[i], false );
	}

	fileSystem->FreeFileList(langFiles);

	Sys_InitScanTable();
}

/*
=================
ReloadLanguage_f
=================
*/
CONSOLE_COMMAND( reloadLanguage, "reload language dict", NULL ) {
	commonLocal.InitLanguageDict();
}

/*
=================
Com_StartBuild_f
=================
*/
CONSOLE_COMMAND( startBuild, "prepares to make a build", NULL ) {
	globalImages->StartBuild();
}

/*
=================
Com_FinishBuild_f
=================
*/
CONSOLE_COMMAND( finishBuild, "finishes the build process", NULL ) {
	if ( game ) {
		game->CacheDictionaryMedia( NULL );
	}
	globalImages->FinishBuild( ( args.Argc() > 1 ) );
}

#ifdef ID_DEDICATED
/*
==============
Com_Help_f
==============
*/
CONSOLE_COMMAND( help, "show help", NULL ) {
	common->Printf( "\nCommonly used commands:\n" );
	common->Printf( "  spawnServer      - start the server.\n" );
	common->Printf( "  disconnect       - shut down the server.\n" );
	common->Printf( "  listCmds         - list all console commands.\n" );
	common->Printf( "  listCVars        - list all console variables.\n" );
	common->Printf( "  kick             - kick a client by number.\n" );
	common->Printf( "  gameKick         - kick a client by name.\n" );
	common->Printf( "  serverNextMap    - immediately load next map.\n" );
	common->Printf( "  serverMapRestart - restart the current map.\n" );
	common->Printf( "  serverForceReady - force all players to ready status.\n" );
	common->Printf( "\nCommonly used variables:\n" );
	common->Printf( "  si_name          - server name (change requires a restart to see)\n" );
	common->Printf( "  si_gametype      - type of game.\n" );
	common->Printf( "  si_fragLimit     - max kills to win (or lives in Last Man Standing).\n" );
	common->Printf( "  si_timeLimit     - maximum time a game will last.\n" );
	common->Printf( "  si_warmup        - do pre-game warmup.\n" );
	common->Printf( "  si_pure          - pure server.\n" );
	common->Printf( "  g_mapCycle       - name of .scriptcfg file for cycling maps.\n" );
	common->Printf( "See mapcycle.scriptcfg for an example of a mapcyle script.\n\n" );
}
#endif

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
	cmdSystem->AddCommand( "debugger", Com_ScriptDebugger_f, CMD_FL_TOOL, "launches the Script Debugger" );

	//BSM Nerve: Add support for the material editor
	cmdSystem->AddCommand( "materialEditor", Com_MaterialEditor_f, CMD_FL_TOOL, "launches the Material Editor" );
#endif
}

/*
=================
idCommonLocal::InitRenderSystem
=================
*/
void idCommonLocal::InitRenderSystem( void ) {
	if ( com_skipRenderer.GetBool() ) {
		return;
	}

	renderSystem->InitOpenGL();
	PrintLoadingMessage( common->GetLanguageDict()->GetString( "#str_04343" ) );
}

/*
=================
idCommonLocal::PrintLoadingMessage
=================
*/
void idCommonLocal::PrintLoadingMessage( const char *msg ) {
	if ( !( msg && *msg ) ) {
		return;
	}
	renderSystem->BeginFrame( renderSystem->GetScreenWidth(), renderSystem->GetScreenHeight() );
	renderSystem->DrawStretchPic( 0, 0, renderSystem->GetVirtualWidth(), renderSystem->GetVirtualHeight(), 0, 0, 1, 1, declManager->FindMaterial( "splashScreen" ) );
	int len = strlen( msg );
	renderSystem->DrawBigStringExt( ( renderSystem->GetVirtualWidth() - len * BIGCHAR_WIDTH ) / 2, renderSystem->GetVirtualHeight() - 80, msg, idVec4( 0.0f, 0.81f, 0.94f, 1.0f ), true );
	renderSystem->EndFrame( NULL, NULL );
}

/*
=================
idCommonLocal::InitSIMD
=================
*/
void idCommonLocal::InitSIMD( void ) {
	idSIMD::InitProcessor( "doom", com_forceGenericSIMD.GetBool() );
	com_forceGenericSIMD.ClearModified();
}

/*
=================
idCommonLocal::GUIFrame
=================
*/
void idCommonLocal::GUIFrame( bool execCmd, bool network ) {
	Sys_GenerateEvents();
	eventLoop->RunEventLoop( execCmd );	// and execute any commands
	com_frameTime = com_ticNumber * USERCMD_MSEC;
	if ( network ) {
		idAsyncNetwork::RunFrame();
	}
	session->Frame();
	session->UpdateScreen( false );
}

/*
=================
idCommonLocal::SingleAsyncTic

The system will asyncronously call this function 60 times a second to
handle the time-critical functions that we don't want limited to
the frame rate:

sound mixing
user input generation (conditioned by com_asyncInput)
packet server operation
packet client operation

We are not using thread safe libraries, so any functionality put here must
be VERY VERY careful about what it calls.
=================
*/

typedef struct {
	int				milliseconds;			// should always be incremeting by 60hz
	int				deltaMsec;				// should always be 16
	int				timeConsumed;			// msec spent in Com_AsyncThread()
	int				clientPacketsReceived;
	int				serverPacketsReceived;
	int				mostRecentServerPacketSequence;
} asyncStats_t;

static const int MAX_ASYNC_STATS = 1024;
asyncStats_t	com_asyncStats[MAX_ASYNC_STATS];		// indexed by com_ticNumber
int prevAsyncMsec;
int	lastTicMsec;

void idCommonLocal::SingleAsyncTic( void ) {
	// main thread code can prevent this from happening while modifying
	// critical data structures
	Sys_EnterCriticalSection();

	asyncStats_t *stat = &com_asyncStats[com_ticNumber & (MAX_ASYNC_STATS-1)];
	memset( stat, 0, sizeof( *stat ) );
	stat->milliseconds = Sys_Milliseconds();
	stat->deltaMsec = stat->milliseconds - com_asyncStats[(com_ticNumber - 1) & (MAX_ASYNC_STATS-1)].milliseconds;

	if ( usercmdGen && com_asyncInput.GetBool() ) {
		usercmdGen->UsercmdInterrupt();
	}

	switch ( com_asyncSound.GetInteger() ) {
		case 1:
		case 3:
			// DG: these are now used for the new default behavior of "update every async tic (every 16ms)"
			soundSystem->AsyncUpdateWrite( stat->milliseconds );
			break;
		case 2:
			// DG: use 2 for the old "update only 10x/second" behavior in case anyone likes that..
			soundSystem->AsyncUpdate( stat->milliseconds );
			break;
	}

	// we update com_ticNumber after all the background tasks
	// have completed their work for this tic
	com_ticNumber++;

	stat->timeConsumed = Sys_Milliseconds() - stat->milliseconds;

	Sys_LeaveCriticalSection();
}

/*
=================
idCommonLocal::Async
=================
*/
void idCommonLocal::Async( void ) {
	int	msec = Sys_Milliseconds();
	if ( !lastTicMsec ) {
		lastTicMsec = msec - USERCMD_MSEC;
	}

	if ( !com_preciseTic.GetBool() ) {
		// just run a single tic, even if the exact msec isn't precise
		SingleAsyncTic();
		return;
	}

	int ticMsec = USERCMD_MSEC;

	// the number of msec per tic can be varies with the timescale cvar
	float timescale = com_timescale.GetFloat();
	if ( timescale != 1.0f ) {
		ticMsec /= timescale;
		if ( ticMsec < 1 ) {
			ticMsec = 1;
		}
	}

	// don't skip too many
	if ( timescale == 1.0f ) {
		if ( lastTicMsec + 10 * USERCMD_MSEC < msec ) {
			lastTicMsec = msec - 10*USERCMD_MSEC;
		}
	}

	while ( lastTicMsec + ticMsec <= msec ) {
		SingleAsyncTic();
		lastTicMsec += ticMsec;
	}
}

/*
=================
idCommonLocal::LoadGameDLLbyName

Helper for LoadGameDLL() to make it less painful to try different dll names.
=================
*/
void idCommonLocal::LoadGameDLLbyName( const char *dll, idStr& s ) {
	s.CapLength(0);
	// try next to the binary first (build tree)
	if (Sys_GetPath(PATH_EXE, s)) {
		// "s = " seems superfluous, but works around g++ 4.7 bug else StripFilename()
		// (and possibly even CapLength()) seems to be "optimized" away and the string contains garbage
		s = s.StripFilename();
		s.AppendPath(dll);
		gameDLL = sys->DLL_Load(s);
	}

	#if defined(_WIN32)
		// then the lib/ dir relative to the binary on windows
		if (!gameDLL && Sys_GetPath(PATH_EXE, s)) {
			s.StripFilename();
			s.AppendPath("lib");
			s.AppendPath(dll);
			gameDLL = sys->DLL_Load(s);
		}
	#else
		// then the install folder on *nix
		if (!gameDLL) {
			s = BUILD_LIBDIR;
			s.AppendPath(dll);
			gameDLL = sys->DLL_Load(s);
		}
	#endif
}

/*
=================
idCommonLocal::LoadGameDLL
=================
*/
void idCommonLocal::LoadGameDLL( void ) {
#ifdef __DOOM_DLL__
	const char		*fs_game;
	char			dll[MAX_OSPATH];
	idStr			s;

	gameImport_t	gameImport;
	gameExport_t	gameExport;
	GetGameAPI_t	GetGameAPI;

	fs_game = cvarSystem->GetCVarString("fs_game");
	if (!fs_game || !fs_game[0])
		fs_game = BASE_GAMEDIR;

	gameDLL = 0;

	sys->DLL_GetFileName(fs_game, dll, sizeof(dll));
	LoadGameDLLbyName(dll, s);

	// there was no gamelib for this mod, use default one from base game
	if (!gameDLL) {
		common->Printf( "\n" );
		common->Warning( "couldn't load mod-specific %s, defaulting to base game's library!\n", dll );
		sys->DLL_GetFileName(BASE_GAMEDIR, dll, sizeof(dll));
		LoadGameDLLbyName(dll, s);
	}

	if ( !gameDLL ) {
		common->FatalError( "couldn't load game dynamic library" );
		return;
	}

	common->Printf("loaded game library '%s'.\n", s.c_str());

	GetGameAPI = (GetGameAPI_t) Sys_DLL_GetProcAddress( gameDLL, "GetGameAPI" );
	if ( !GetGameAPI ) {
		Sys_DLL_Unload( gameDLL );
		gameDLL = 0;
		common->FatalError( "couldn't find game DLL API" );
		return;
	}

	gameImport.version					= GAME_API_VERSION;
	gameImport.sys						= ::sys;
	gameImport.common					= ::common;
	gameImport.cmdSystem				= ::cmdSystem;
	gameImport.cvarSystem				= ::cvarSystem;
	gameImport.fileSystem				= ::fileSystem;
	gameImport.networkSystem			= ::networkSystem;
	gameImport.renderSystem				= ::renderSystem;
	gameImport.soundSystem				= ::soundSystem;
	gameImport.renderModelManager		= ::renderModelManager;
	gameImport.uiManager				= ::uiManager;
	gameImport.declManager				= ::declManager;
	gameImport.AASFileManager			= ::AASFileManager;
	gameImport.collisionModelManager	= ::collisionModelManager;

	gameExport							= *GetGameAPI( &gameImport);

	if ( gameExport.version != GAME_API_VERSION ) {
		Sys_DLL_Unload( gameDLL );
		gameDLL = 0;
		common->FatalError( "wrong game DLL API version" );
		return;
	}

	game								= gameExport.game;
	gameEdit							= gameExport.gameEdit;

#endif

	// initialize the game object
	if ( game != NULL ) {
		game->Init();
	}
}

/*
=================
idCommonLocal::UnloadGameDLL
=================
*/
void idCommonLocal::UnloadGameDLL( void ) {

	// shut down the game object
	if ( game != NULL ) {
		game->Shutdown();
	}

#ifdef __DOOM_DLL__

	if ( gameDLL ) {
		Sys_DLL_Unload( gameDLL );
		gameDLL = 0;
	}
	game = NULL;
	gameEdit = NULL;

#endif

	com_debuggerSupported = false; // HvG: Reset debugger availability.
	gameCallbacks.Reset(); // DG: these callbacks are invalid now because DLL has been unloaded
}

/*
=================
idCommonLocal::IsInitialized
=================
*/
bool idCommonLocal::IsInitialized( void ) const {
	return com_fullyInitialized;
}

/*
=================
idCommonLocal::SetMachineSpec
=================
*/
void idCommonLocal::SetMachineSpec( void ) {
	int sysRam = Sys_GetSystemRam();

	Printf( "Detected\n\t%i MB of System memory\n\n", sysRam );

	if ( sysRam >= 1024 ) {
		Printf( "This system qualifies for Ultra quality!\n" );
		com_imageQuality.SetInteger( 3 );
	} else if ( sysRam >= 512 ) {
		Printf( "This system qualifies for High quality!\n" );
		com_imageQuality.SetInteger( 2 );
	} else if ( sysRam >= 384 ) {
		Printf( "This system qualifies for Medium quality.\n" );
		com_imageQuality.SetInteger( 1 );
	} else {
		Printf( "This system qualifies for Low quality.\n" );
		com_imageQuality.SetInteger( 0 );
	}
}

static unsigned int AsyncTimer(unsigned int interval, void *) {
	common->Async();
	Sys_TriggerEvent(TRIGGER_EVENT_ONE);

	// calculate the next interval to get as close to 60fps as possible
	unsigned int now = SDL_GetTicks();
	unsigned int tick = com_ticNumber * USERCMD_MSEC;
	// FIXME: this is pretty broken and basically always returns 1 because now now is much bigger than tic
	//        (probably com_tickNumber only starts incrementing a second after engine starts?)
	//        only reason this works is common->Async() checking again before calling SingleAsyncTic()

	if (now >= tick)
		return 1;

	return tick - now;
}

#ifdef _WIN32
#include "../sys/win32/win_local.h" // for Conbuf_AppendText()
#endif // _WIN32

static bool checkForHelp(int argc, char **argv)
{
	const char* helpArgs[] = { "--help", "-h", "-help", "-?", "/?" };
	const int numHelpArgs = sizeof(helpArgs)/sizeof(helpArgs[0]);

	for (int i=0; i<argc; ++i)
	{
		const char* arg = argv[i];
		for (int h=0; h<numHelpArgs; ++h)
		{
			if (idStr::Icmp(arg, helpArgs[h]) == 0)
			{
#ifdef _WIN32
				// write it to the Windows-only console window
				#define WriteString(s) Conbuf_AppendText(s)
#else // not windows
				// write it to stdout
				#define WriteString(s) fputs(s, stdout);
#endif // _WIN32
				WriteString(ENGINE_VERSION " - http://dhewm3.org\n");
				WriteString("Commandline arguments:\n");
				WriteString("-h or --help: Show this help\n");
				WriteString("+<command> [command arguments]\n");
				WriteString("  executes a command (with optional arguments)\n");

				WriteString("\nSome interesting commands:\n");
				WriteString("+map <map>\n");
				WriteString("  directly loads the given level, e.g. +map game/hell1\n");
				WriteString("+exec <config>\n");
				WriteString("  execute the given config (mainly relevant for dedicated servers)\n");
				WriteString("+disconnect\n");
				WriteString("  starts the game, goes directly into main menu without showing\n  logo video\n");
				WriteString("+connect <host>[:port]\n");
				WriteString("  directly connect to multiplayer server at given host/port\n");
				WriteString("  e.g. +connect d3.example.com\n");
				WriteString("  e.g. +connect d3.example.com:27667\n");
				WriteString("  e.g. +connect 192.168.0.42:27666\n");
				WriteString("+set <cvarname> <value>\n");
				WriteString("  Set the given cvar to the given value, e.g. +set r_fullscreen 0\n");
				WriteString("+seta <cvarname> <value>\n");
				WriteString("  like +set, but also makes sure the changed cvar is saved (\"archived\")\n  in a cfg\n");

				WriteString("\nSome interesting cvars:\n");
				WriteString("+set fs_basepath <gamedata path>\n");
				WriteString("  set path to your Doom3 game data (the directory base/ is in)\n");
				WriteString("+set fs_game <modname>\n");
				WriteString("  start the given addon/mod, e.g. +set fs_game d3xp\n");
				WriteString("+set fs_game_base <base-modname>\n");
				WriteString("  some mods are based on other mods, usually d3xp.\n");
				WriteString("  This specifies the base mod e.g. +set fs_game d3le +set fs_game_base d3xp\n");
#ifndef ID_DEDICATED
				WriteString("+set r_fullscreen <0 or 1>\n");
				WriteString("  start game in windowed (0) or fullscreen (1) mode\n");
				WriteString("+set r_mode <modenumber>\n");
				WriteString("  start game in resolution belonging to <modenumber>,\n");
				WriteString("  use -1 for custom resolutions:\n");
				WriteString("+set r_customWidth  <size in pixels>\n");
				WriteString("+set r_customHeight <size in pixels>\n");
				WriteString("  if r_mode is set to -1, these cvars allow you to specify the\n");
				WriteString("  width/height of your custom resolution\n");
#endif // !ID_DEDICATED
				WriteString("\nSee https://modwiki.dhewm3.org/CVars_%28Doom_3%29 for more cvars\n");
				WriteString("See https://modwiki.dhewm3.org/Commands_%28Doom_3%29 for more commands\n");

				#undef WriteString

				return true;
			}
		}
	}
	return false;
}

#ifdef UINTPTR_MAX // DG: make sure D3_SIZEOFPTR is consistent with reality

#if D3_SIZEOFPTR == 4
  #if UINTPTR_MAX != 0xFFFFFFFFUL
	#error "CMake assumes that we're building for a 32bit architecture, but UINTPTR_MAX doesn't match!"
  #endif
#elif D3_SIZEOFPTR == 8
  #if UINTPTR_MAX != 18446744073709551615ULL
	#error "CMake assumes that we're building for a 64bit architecture, but UINTPTR_MAX doesn't match!"
  #endif
#else
  // Hello future person with a 128bit(?) CPU, I hope the future doesn't suck too much and that you don't still use CMake.
  // Also, please adapt this check and send a pull request (or whatever way we have to send patches in the future)
  #error "D3_SIZEOFPTR should really be 4 (for 32bit targets) or 8 (for 64bit targets), what kind of machine is this?!"
#endif

#endif // UINTPTR_MAX defined

/*
=================
idCommonLocal::Init
=================
*/
void idCommonLocal::Init( int argc, char **argv ) {

	// in case UINTPTR_MAX isn't defined (or wrong), do a runtime check at startup
	if ( D3_SIZEOFPTR != sizeof(void*) ) {
		Sys_Error( "Something went wrong in your build: CMake assumed that sizeof(void*) == %d but in reality it's %d!\n",
				   (int)D3_SIZEOFPTR, (int)sizeof(void*) );
	}

	if ( checkForHelp( argc, argv ) ) {
		// game has been started with --help (or similar), usage message has been shown => quit
#ifdef _WIN32
		// this enforces that the console window is shown until the user closes it
		// => checkForHelp() writes to the console window on Windows
		Sys_Error( "." );
#endif // _WIN32
		exit(1);
	}

#ifdef ID_DEDICATED
	// we want to use the SDL event queue for dedicated servers. That
	// requires video to be initialized, so we just use the dummy
	// driver for headless boxen
#if SDL_VERSION_ATLEAST(2, 0, 0)
	SDL_setenv("SDL_VIDEODRIVER", "dummy", 1);
#else
	char dummy[] = "SDL_VIDEODRIVER=dummy\0";
	SDL_putenv(dummy);
#endif
#endif

	if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_JOYSTICK)) // init joystick to work around SDL 2.0.9 bug #4391
		Sys_Error("Error while initializing SDL: %s", SDL_GetError());

	Sys_InitThreads();

#if SDL_VERSION_ATLEAST(2, 0, 0)
	/* Force the window to minimize when focus is lost. This was the
	 * default behavior until SDL 2.0.12 and changed with 2.0.14.
	 * The windows staying maximized has some odd implications for
	 * window ordering under Windows and some X11 window managers
	 * like kwin. See:
	 *  * https://github.com/libsdl-org/SDL/issues/4039
	 *  * https://github.com/libsdl-org/SDL/issues/3656 */
	SDL_SetHint( SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "1" );
#endif

	try {

		// set interface pointers used by idLib
		idLib::sys			= sys;
		idLib::common		= common;
		idLib::cvarSystem	= cvarSystem;
		idLib::fileSystem	= fileSystem;

		// initialize idLib
		idLib::Init();

		// clear warning buffer
		ClearWarnings( GAME_NAME " initialization" );

		// parse command line options
		ParseCommandLine( argc, argv );

		// init console command system
		cmdSystem->Init();

		// init CVar system
		cvarSystem->Init();

		// start file logging right away, before early console or whatever
		StartupVariable( "win_outputDebugString", false );

		// register all static CVars
		idCVar::RegisterStaticVars();

		// print engine version
#if SDL_VERSION_ATLEAST(2, 0, 0)
		SDL_version sdlv;
		SDL_GetVersion(&sdlv);
#else
		SDL_version sdlv = *SDL_Linked_Version();
#endif
		Printf( "%s using SDL v%u.%u.%u\n",
				version.string, sdlv.major, sdlv.minor, sdlv.patch );

#if SDL_VERSION_ATLEAST(2, 0, 0)
		Printf( "SDL video driver: %s\n", SDL_GetCurrentVideoDriver() );
#endif

		// initialize key input/binding, done early so bind command exists
		idKeyInput::Init();

		// init the console so we can take prints
		console->Init();

		// get architecture info
		Sys_Init();

		// initialize networking
		Sys_InitNetworking();

		// override cvars from command line
		StartupVariable( NULL, false );

		// set fpu double extended precision
		Sys_FPU_SetPrecision();

		// initialize processor specific SIMD implementation
		InitSIMD();

		// init commands
		InitCommands();

#ifdef ID_WRITE_VERSION
		config_compressor = idCompressor::AllocArithmetic();
#endif

		// game specific initialization
		InitGame();

		// don't add startup commands if no CD key is present
#if ID_ENFORCE_KEY
		if ( !session->CDKeysAreValid( false ) || !AddStartupCommands() ) {
#else
		if ( !AddStartupCommands() ) {
#endif
			// if the user didn't give any commands, run default action
			session->StartMenu( true );
		}

		// print all warnings queued during initialization
		PrintWarnings();

#ifdef	ID_DEDICATED
		Printf( "\nType 'help' for dedicated server info.\n\n" );
#endif

		// remove any prints from the notify lines
		console->ClearNotifyLines();

		ClearCommandLine();

		// load the console history file
		consoleHistory.LoadHistoryFile();

		com_fullyInitialized = true;
	}

	catch( idException & ) {
		Sys_Error( "Error during initialization" );
	}

	async_timer = SDL_AddTimer(USERCMD_MSEC, AsyncTimer, NULL);

	if (!async_timer)
		Sys_Error("Error while starting the async timer: %s", SDL_GetError());
}


/*
=================
idCommonLocal::Shutdown
=================
*/
void idCommonLocal::Shutdown( void ) {
	if (async_timer) {
		SDL_RemoveTimer(async_timer);
		async_timer = 0;
	}

	idAsyncNetwork::server.Kill();
	idAsyncNetwork::client.Shutdown();

	// game specific shut down
	ShutdownGame( false );

	// shut down non-portable system services
	Sys_Shutdown();

	// shut down the console
	console->Shutdown();

	// shut down the key system
	idKeyInput::Shutdown();

	// shut down the cvar system
	cvarSystem->Shutdown();

	// shut down the console command system
	cmdSystem->Shutdown();

#ifdef ID_WRITE_VERSION
	delete config_compressor;
	config_compressor = NULL;
#endif

	// free any buffered warning messages
	ClearWarnings( GAME_NAME " shutdown" );
	warningCaption.Clear();
	errorList.Clear();

	// free language dictionary
	languageDict.Clear();

	// enable leak test
	Mem_EnableLeakTest( "doom" );

	// shutdown idLib
	idLib::ShutDown();

	Sys_ShutdownThreads();

	SDL_Quit();
}

/*
=================
idCommonLocal::InitGame
=================
*/
void idCommonLocal::InitGame( void ) {
	// initialize the file system
	fileSystem->Init();

	// initialize the declaration manager
	declManager->Init();

	// force r_fullscreen 0 if running a tool
	CheckToolMode();

	idFile *file = fileSystem->OpenExplicitFileRead( fileSystem->RelativePathToOSPath( CONFIG_SPEC, "fs_configpath" ) );
	bool sysDetect = ( file == NULL );
	if ( file ) {
		fileSystem->CloseFile( file );
	} else {
		file = fileSystem->OpenFileWrite( CONFIG_SPEC, "fs_configpath" );
		fileSystem->CloseFile( file );
	}

	idCmdArgs args;
	if ( sysDetect ) {
		SetMachineSpec();
		cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "execMachineSpec %s", args ) );
	}

	// initialize the renderSystem data structures, but don't start OpenGL yet
	renderSystem->Init();

	// initialize string database right off so we can use it for loading messages
	InitLanguageDict();

	PrintLoadingMessage( common->GetLanguageDict()->GetString( "#str_04344" ) );

	// init journalling, etc
	eventLoop->Init();

	PrintLoadingMessage( common->GetLanguageDict()->GetString( "#str_04345" ) );

	// exec the startup scripts
	cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "exec editor.cfg\n" );
	cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "exec default.cfg\n" );

	// skip the config file if "safe" is on the command line
	if ( !SafeMode() ) {
		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "exec " CONFIG_FILE "\n" );
	}
	cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "exec autoexec.cfg\n" );

	// reload the language dictionary now that we've loaded config files
	cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "reloadLanguage\n" );

	// run cfg execution
	cmdSystem->ExecuteCommandBuffer();

	// re-override anything from the config files with command line args
	StartupVariable( NULL, false );

	// if any archived cvars are modified after this, we will trigger a writing of the config file
	cvarSystem->ClearModifiedFlags( CVAR_ARCHIVE );

	// init the user command input code
	usercmdGen->Init();

	PrintLoadingMessage( common->GetLanguageDict()->GetString( "#str_04346" ) );

	// start the sound system, but don't do any hardware operations yet
	soundSystem->Init();

	PrintLoadingMessage( common->GetLanguageDict()->GetString( "#str_04347" ) );

	// init async network
	idAsyncNetwork::Init();

#ifdef	ID_DEDICATED
	idAsyncNetwork::server.InitPort();
	cvarSystem->SetCVarBool( "s_noSound", true );
#else
	if ( idAsyncNetwork::serverDedicated.GetInteger() == 1 ) {
		idAsyncNetwork::server.InitPort();
		cvarSystem->SetCVarBool( "s_noSound", true );
	} else {
		// init OpenGL, which will open a window and connect sound and input hardware
		PrintLoadingMessage( common->GetLanguageDict()->GetString( "#str_04348" ) );
		InitRenderSystem();
	}
#endif

	PrintLoadingMessage( common->GetLanguageDict()->GetString( "#str_04349" ) );

	// initialize the user interfaces
	uiManager->Init();

	PrintLoadingMessage( common->GetLanguageDict()->GetString( "#str_04350" ) );

	// load the game dll
	LoadGameDLL();

	// startup the script debugger
	if ( com_enableDebuggerServer.GetBool( ) )
		DebuggerServerInit( );

	PrintLoadingMessage( common->GetLanguageDict()->GetString( "#str_04351" ) );

	// init the session
	session->Init();

	// have to do this twice.. first one sets the correct r_mode for the renderer init
	// this time around the backend is all setup correct.. a bit fugly but do not want
	// to mess with all the gl init at this point.. an old vid card will never qualify for
	if ( sysDetect ) {
		SetMachineSpec();
		cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "execMachineSpec %s", args ) );
		cvarSystem->SetCVarInteger( "s_numberOfSpeakers", 6 );
		cmdSystem->BufferCommandText( CMD_EXEC_NOW, "s_restart\n" );
		cmdSystem->ExecuteCommandBuffer();
	}
}

/*
=================
idCommonLocal::ShutdownGame
=================
*/
void idCommonLocal::ShutdownGame( bool reloading ) {

	// kill sound first
	idSoundWorld *sw = soundSystem->GetPlayingSoundWorld();
	if ( sw ) {
		sw->StopAllSounds();
	}

	// shutdown the script debugger
	if ( com_enableDebuggerServer.GetBool() )	
		DebuggerServerShutdown();

	idAsyncNetwork::client.Shutdown();

	// shut down the session
	session->Shutdown();

	// shut down the user interfaces
	uiManager->Shutdown();

	// shut down the sound system
	soundSystem->Shutdown();

	// shut down async networking
	idAsyncNetwork::Shutdown();

	// shut down the user command input code
	usercmdGen->Shutdown();

	// shut down the event loop
	eventLoop->Shutdown();

	// shut down the renderSystem
	renderSystem->Shutdown();

	// shutdown the decl manager
	declManager->Shutdown();

	// unload the game dll
	UnloadGameDLL();

	// dump warnings to "warnings.txt"
#ifdef DEBUG
	DumpWarnings();
#endif

	// shut down the file system
	fileSystem->Shutdown( reloading );
}

/*
=================
ShowMemoryUsage_f
=================
*/
CONSOLE_COMMAND( memoryDump, "creates a memory dump", NULL ) {
	Mem_DumpCompressed_f( args );
}

/*
=================
ShowMemoryUsage_f
=================
*/
CONSOLE_COMMAND( memoryDumpCompressed, "creates a compressed memory dump", NULL ) {
	Mem_DumpCompressed_f( args );
}

/*
=================
ShowMemoryUsage_f
=================
*/
CONSOLE_COMMAND( showStringMemory, "shows memory used by strings", NULL ) {
	idStr::ShowMemoryUsage_f( args );
}

/*
=================
ShowMemoryUsage_f
=================
*/
CONSOLE_COMMAND( showDictMemory, "shows memory used by dictionaries", NULL ) {
	idDict::ShowMemoryUsage_f( args );
}

/*
=================
ListKeys_f
=================
*/
CONSOLE_COMMAND( listDictKeys, "lists all keys used by dictionaries", NULL ) {
	idDict::ListKeys_f( args );
}

/*
=================
ListValues_f
=================
*/
CONSOLE_COMMAND( listDictValues, "lists all values used by dictionaries", NULL ) {
	idDict::ListValues_f( args );
}

/*
=================
Test_f
=================
*/
CONSOLE_COMMAND( testSIMD, "test SIMD code", NULL ) {
	idSIMD::Test_f( args );
}


// DG: below here are hacks to allow adding callbacks and exporting additional functions to the
//     Game DLL without breaking the ABI. See Common.h for longer explanation...


// returns true if setting the callback was successful, else false
// When a game DLL is unloaded the callbacks are automatically removed from the Engine
// so you usually don't have to worry about that; but you can call this with cb = NULL
// and userArg = NULL to remove a callback manually (e.g. if userArg refers to an object you deleted)
bool idCommonLocal::SetCallback(idCommon::CallbackType cbt, idCommon::FunctionPointer cb, void* userArg)
{
	switch(cbt)
	{
		case idCommon::CB_ReloadImages:
			gameCallbacks.reloadImagesCB = (idGameCallbacks::ReloadImagesCallback)cb;
			gameCallbacks.reloadImagesUserArg = userArg;
			return true;

		default:
			Warning("Called idCommon::SetCallback() with unknown CallbackType %d!\n", cbt);
			return false;
	}
}

static bool isDemo( void )
{
	return sessLocal.IsDemoVersion();
}

static bool updateDebugger( idInterpreter *interpreter, idProgram *program, int instructionPointer )
{
	if (com_editors & EDITOR_DEBUGGER) 
	{
		DebuggerServerCheckBreakpoint( interpreter, program, instructionPointer );
		return true;
	}
	return false;
}

// returns true if that function is available in this version of dhewm3
// *out_fnptr will be the function (you'll have to cast it probably)
// *out_userArg will be an argument you have to pass to the function, if appropriate (else NULL)
bool idCommonLocal::GetAdditionalFunction(idCommon::FunctionType ft, idCommon::FunctionPointer* out_fnptr, void** out_userArg)
{
	if(out_userArg != NULL)
		*out_userArg = NULL;

	if(out_fnptr == NULL)
	{
		Warning("Called idCommon::GetAdditionalFunction() with out_fnptr == NULL!\n");
		return false;
	}

	switch(ft)
	{
		case idCommon::FT_IsDemo:
			*out_fnptr = (idCommon::FunctionPointer)isDemo;
			// don't set *out_userArg, this function takes no arguments
			return true;

		case idCommon::FT_UpdateDebugger:
			*out_fnptr = (idCommon::FunctionPointer)updateDebugger;
			com_debuggerSupported = true;
			return true;

		default:
			*out_fnptr = NULL;
			Warning("Called idCommon::SetCallback() with unknown FunctionType %d!\n", ft);
			return false;
	}
}

idGameCallbacks gameCallbacks;

idGameCallbacks::idGameCallbacks()
: reloadImagesCB(NULL), reloadImagesUserArg(NULL)
{}

void idGameCallbacks::Reset()
{
	reloadImagesCB = NULL;
	reloadImagesUserArg = NULL;
}
