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

/*
===============================================================================

	Preprocessor settings for compiling different versions.

===============================================================================
*/

// memory debugging
//#define ID_REDIRECT_NEWDELETE
//#define ID_DEBUG_MEMORY
//#define ID_DEBUG_UNINITIALIZED_MEMORY

// HUMANHEAD mdl:  Moved this below so we can see if _ENABLE_GOLD_ is defined
// if enabled, the console won't toggle upon ~, unless you start the binary with +set com_allowConsole 1
// Ctrl+Alt+~ will always bring the console no matter what
//#define ID_CONSOLE_LOCK 0	// HUMANHEAD pdm
//#ifndef ID_CONSOLE_LOCK
//	#if defined(_WIN32) || defined(MACOS_X)
//		#ifdef _DEBUG
//			#define ID_CONSOLE_LOCK 0
//		#else
//			#define ID_CONSOLE_LOCK 1
//		#endif
//	#else
//		#define ID_CONSOLE_LOCK 0
//	#endif
//#endif
// HUMANHEAD END

// useful for network debugging, turns off 'LAN' checks, all IPs are classified 'internet'
#ifndef ID_NOLANADDRESS
	#define ID_NOLANADDRESS 0
#endif

#ifndef ID_VERSIONTAG
	#define ID_VERSIONTAG ""
	//#define ID_VERSIONTAG ".MP"
#endif

// let .dds be loaded from FS without altering pure state. only for developement.
#ifndef ID_PURE_ALLOWDDS
	#define ID_PURE_ALLOWDDS 0
#endif

// build an exe with no CVAR_CHEAT controls
#ifndef ID_ALLOW_CHEATS
	#define ID_ALLOW_CHEATS 0
#endif

// verify checksums in clientinfo traffic
// NOTE: this makes the network protocol incompatible
#ifndef ID_CLIENTINFO_TAGS
	#define ID_CLIENTINFO_TAGS 0
#endif

// for win32 this is defined in preprocessor settings so that MFC can be
// compiled out.
//#define ID_DEDICATED

// don't define ID_ALLOW_TOOLS when we don't want tool code in the executable. - DG: defined in cmake now
#if 0 // defined( _WIN32 ) && defined(_MFC_VER) && !defined( ID_DEDICATED )
	#define	ID_ALLOW_TOOLS
#endif

//#define ID_ENFORCE_KEY 0

// HUMANHEAD mdl:  Moved to licensee.h
//#ifndef ID_ENFORCE_KEY
//#	if !defined( ID_DEDICATED )
//#		define ID_ENFORCE_KEY 1
//#	else
//#		define ID_ENFORCE_KEY 0
//#	endif
//#endif // HUMANHEAD END

//#ifndef ID_ENFORCE_KEY_CLIENT
//#	if ID_ENFORCE_KEY
//#		define ID_ENFORCE_KEY_CLIENT 1
//#	else
//#		define ID_ENFORCE_KEY_CLIENT 0
//#	endif
//#endif

// HUMANHEAD tmj: build automation will search and replace these if necessary
// #define _EXPIRE_
// #define _NODONGLE_
// #define _SYSTEM_BUILD_
// #define _3DR_BUILD_
// #define _T2_BUILD_
// #define _VENOM_BUILD_
// #define _ATI_BUILD_
// #define _IBETA_BUILD_
// #define _NVIDIA_BUILD_
// #define _CREATIVE_BUILD_
// #define _ENABLE_GOLD_
// #define _DISCLAIMER_
// #define _USE_SECUROM_
// #define _HH_DEMO_BUILD_
// #define _GERMAN_BUILD_
// #define _LOCALIZATION_BUILD_
// #define _DISTREAM_BUILD_
// #define _MOBILE_BUILD_
// #define _INTERNAL_BUILD_

// if enabled, the console won't toggle upon ~, unless you start the binary with +set com_allowConsole 1
// Ctrl+Alt+~ will always bring the console no matter what
#ifdef _ENABLE_GOLD_

#ifndef ID_CONSOLE_LOCK
	#if !defined _INTERNAL_BUILD_ && ( defined(_WIN32) || defined(MACOS_X) )
		#ifdef _DEBUG
			#define ID_CONSOLE_LOCK 0
		#else
			#define ID_CONSOLE_LOCK 1
		#endif
	#else
		#define ID_CONSOLE_LOCK 0
	#endif
#endif

#else // _ENABLE_GOLD_
#define ID_CONSOLE_LOCK 0
#endif
// HUMANHEAD END

// END HUMANHEAD

// async network

/*
DOOM III gold:	33
1.1 beta patch:	34
1.1 patch:		35
1.2 XP:			36-39
1.3 patch:		40
1.3.1:			41

dhewm WIP		42
*/
#define ASYNC_PROTOCOL_MINOR	(42)
#define ASYNC_PROTOCOL_VERSION	(( ASYNC_PROTOCOL_MAJOR << 16 ) + ASYNC_PROTOCOL_MINOR)

#define MAX_ASYNC_CLIENTS		(32)

#define MAX_USERCMD_BACKUP		(256)
#define MAX_USERCMD_DUPLICATION	(25)
#define MAX_USERCMD_RELAY		(10)

// index 0 is hardcoded to be the idnet master
// which leaves 4 to user customization
#define MAX_MASTER_SERVERS		(5)

#define MAX_NICKLEN				(32)

// max number of servers that will be scanned for at a single IP address
#define MAX_SERVER_PORTS		(8)

// special game init ids
#define GAME_INIT_ID_INVALID	(-1)
#define GAME_INIT_ID_MAP_LOAD	(-2)

// threads

#define MAX_THREADS				(10)
