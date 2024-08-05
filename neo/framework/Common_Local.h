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

#ifndef __COMMON_LOCAL_H__
#define __COMMON_LOCAL_H__

#include <SDL.h>

typedef enum {
	ERP_NONE,
	ERP_FATAL,						// exit the entire game with a popup window
	ERP_DROP,						// print to console and disconnect from game
	ERP_DISCONNECT					// don't kill server
} errorParm_t;

#define	MAX_PRINT_MSG_SIZE	4096
#define MAX_WARNING_LIST	256

/*
==============================================================

  Common Local

==============================================================
*/

class idCommonLocal : public idCommon {
public:
								idCommonLocal( void );

	virtual void				Init( int argc, char **argv );
	virtual void				Shutdown( void );
	virtual void				Quit( void );
	virtual bool				IsInitialized( void ) const;
	virtual void				Frame( void );
	virtual void				GUIFrame( bool execCmd, bool network );
	virtual void				Async( void );
	virtual void				StartupVariable( const char *match, bool once );
	virtual void				InitTool( const toolFlag_t tool, const idDict *dict );
	virtual void				ActivateTool( bool active );
	virtual void				WriteConfigToFile( const char *filename );
	virtual void				WriteFlaggedCVarsToFile( const char *filename, int flags, const char *setCmd );
	virtual void				BeginRedirect( char *buffer, int buffersize, void (*flush)( const char * ) );
	virtual void				EndRedirect( void );
	virtual void				SetRefreshOnPrint( bool set );
	virtual void				Printf( const char *fmt, ... ) id_attribute((format(printf,2,3)));
	virtual void				VPrintf( const char *fmt, va_list arg );
	virtual void				DPrintf( const char *fmt, ... ) id_attribute((format(printf,2,3)));
	virtual void				Warning( const char *fmt, ... ) id_attribute((format(printf,2,3)));
	virtual void				DWarning( const char *fmt, ...) id_attribute((format(printf,2,3)));
	virtual void				PrintWarnings( void );
	virtual void				ClearWarnings( const char *reason );
	virtual void				Error( const char *fmt, ... ) id_attribute((format(printf,2,3)));
	virtual void				FatalError( const char *fmt, ... ) id_attribute((format(printf,2,3)));
	virtual const idLangDict *	GetLanguageDict( void );

	virtual const char *		KeysFromBinding( const char *bind );
	virtual const char *		BindingFromKey( const char *key );

	virtual int					ButtonState( int key );
	virtual int					KeyState( int key );

	virtual void				FixupKeyTranslations( const char *src, char *dst, int lengthAllocated) { (void) src; (void)dst; (void)lengthAllocated; }
	virtual void				MaterialKeyForBinding (const char *binding, char *keyMaterial, char *key, bool &isBound ) {
		(void)binding; (void)keyMaterial; (void)key;
		isBound = false;
	}
	virtual void				SetGameSensitivityFactor( float factor ) { (void) factor; }

	virtual void				APrintf( const char *fmt, ... ) {}	// Another print interface for animation module
	virtual double				GetAsyncTime() { return 0; };						// Method for safely retrieving async profile info
	virtual bool				CanUseOpenAL( void ) { return true; }

	// DG: hack to allow adding callbacks and exporting additional functions without breaking the game ABI
	//     see Common.h for longer explanation...

	// returns true if setting the callback was successful, else false
	// When a game DLL is unloaded the callbacks are automatically removed from the Engine
	// so you usually don't have to worry about that; but you can call this with cb = NULL
	// and userArg = NULL to remove a callback manually (e.g. if userArg refers to an object you deleted)
	virtual bool				SetCallback(idCommon::CallbackType cbt, idCommon::FunctionPointer cb, void* userArg);

	// returns true if that function is available in this version of dhewm3
	// *out_fnptr will be the function (you'll have to cast it probably)
	// *out_userArg will be an argument you have to pass to the function, if appropriate (else NULL)
	// NOTE: this doesn't do anything yet, but allows to add ugly mod-specific hacks without breaking the Game interface
	virtual bool				GetAdditionalFunction(idCommon::FunctionType ft, idCommon::FunctionPointer* out_fnptr, void** out_userArg);

	// DG end

	void						InitGame( void );
	void						ShutdownGame( bool reloading );

	// localization
	void						InitLanguageDict( void );
	void						LocalizeGui( const char *fileName, idLangDict &langDict );
	void						LocalizeMapData( const char *fileName, idLangDict &langDict );
	void						LocalizeSpecificMapData( const char *fileName, idLangDict &langDict, const idLangDict &replaceArgs );

	void						SetMachineSpec( void );

private:
	void						InitCommands( void );
	void						InitRenderSystem( void );
	void						InitSIMD( void );
	bool						AddStartupCommands( void );
	void						ParseCommandLine( int argc, char **argv );
	void						ClearCommandLine( void );
	bool						SafeMode( void );
	void						CheckToolMode( void );
	void						WriteConfiguration( void );
	void						DumpWarnings( void );
	void						SingleAsyncTic( void );
	void						LoadGameDLL( void );
	void						LoadGameDLLbyName( const char *dll, idStr& s );
	void						UnloadGameDLL( void );
	void						PrintLoadingMessage( const char *msg );
	void						FilterLangList( idStrList* list, idStr lang );

	bool						com_fullyInitialized;
	bool						com_refreshOnPrint;		// update the screen every print for dmap
	int							com_errorEntered;		// 0, ERP_DROP, etc

	char						errorMessage[MAX_PRINT_MSG_SIZE];

	char *						rd_buffer;
	int							rd_buffersize;
	void						(*rd_flush)( const char *buffer );

	idStr						warningCaption;
	idStrList					warningList;
	idStrList					errorList;

	uintptr_t					gameDLL;

	idLangDict					languageDict;

#ifdef ID_WRITE_VERSION
	idCompressor *				config_compressor;
#endif

	SDL_TimerID					async_timer;
};

extern idCommonLocal commonLocal;

#endif /* !__COMMON_LOCAL_H__ */
