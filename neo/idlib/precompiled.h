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

#ifndef __PRECOMPILED_H__
#define __PRECOMPILED_H__

#ifdef _WIN32

#if defined(ID_ALLOW_TOOLS) && !defined(_D3SDK) && !defined(GAME_DLL)
// (hopefully) suppress "warning C4996: 'MBCS_Support_Deprecated_In_MFC':
//   MBCS support in MFC is deprecated and may be removed in a future version of MFC."
#define NO_WARN_MBCS_MFC_DEPRECATION

#include <afxwin.h>

#include "../tools/comafx/framework.h"
#include "../tools/comafx/pch.h"

// scaling factor based on DPI (dpi/96.0f, so 1.0 by default); implemented in win_main.cpp
float Win_GetWindowScalingFactor(HWND window);
#endif

#include <winsock2.h>
#include <mmsystem.h>
#include <mmreg.h>

#include <malloc.h>							// no malloc.h on mac or unix

#endif /* _WIN32 */

//-----------------------------------------------------

#if !defined( _DEBUG ) && !defined( NDEBUG )
	// don't generate asserts
	#define NDEBUG
#endif

//-----------------------------------------------------

// configuration
#include "config.h"

//HUMANHEAD rww - moved up from "framework"
#include "../framework/BuildVersion.h" // HUMANHEAD mdl:  Removed from precompiled headers and put in individual cpp files as needed.
#include "../framework/BuildDefines.h"
#include "../framework/Licensee.h"
//HUMANHEAD END

// non-portable system services
#include "../sys/platform.h"
#include "../sys/sys_public.h"
#include "../sys/sys_sdl.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../sys/sys_imgui.h"

// id lib
#include "../idlib/Lib.h"

// framework
#include "../framework/CmdSystem.h"
#include "../framework/CVarSystem.h"
#include "../framework/Common.h"
#include "../framework/File.h"
#include "../framework/FileSystem.h"
#include "../framework/UsercmdGen.h"

// decls
#include "../framework/DeclManager.h"
#include "../framework/DeclTable.h"
#include "../framework/DeclSkin.h"
#include "../framework/DeclEntityDef.h"
#include "../framework/DeclFX.h"
#include "../framework/DeclParticle.h"
#include "../framework/DeclPreyBeam.h" // HUMANHEAD CJR
#include "../framework/DeclAF.h"

//HUMANHEAD rww - for memory build
#ifdef ID_REDIRECT_NEWDELETE
#undef new
void *						operator new( size_t );
void *						operator new( size_t s, int, int, char *, int );
void						operator delete( void * );
void						operator delete( void *, int, int, char *, int );
#endif
//HUMANHEAD END

// We have expression parsing and evaluation code in multiple places:
// materials, sound shaders, and guis. We should unify them.
const int MAX_EXPRESSION_OPS = 4096;
const int MAX_EXPRESSION_REGISTERS = 4096;

// renderer
#include "renderer/qgl.h"
#include "renderer/Cinematic.h"
#include "renderer/Material.h"
#include "renderer/Model.h"
#include "renderer/ModelManager.h"
#include "renderer/DeclRenderProg.h"
#include "renderer/RenderSystem.h"
#include "renderer/RenderWorld.h"
#include "renderer/DeviceContext.h"

// sound engine
#include "../sound/sound.h"

// asynchronous networking
#include "../framework/async/NetworkSystem.h"

// user interfaces
#include "../ui/ListGUI.h"
#include "../ui/UserInterface.h"

// collision detection system
#include "../cm/CollisionModel.h"

// AAS files and manager
#include "../aas/AASFile.h"
#include "../aas/AASFileManager.h"

// MayaImport
#include "../MayaImport/maya_main.h"

//HUMANHEAD
#include "../preyengine/profiler.h"			// Exposed to both engine and game
											// Must be after engine systems, but before game.h
//HUMANHEAD END

// game interface
#include "../framework/Game.h"

//-----------------------------------------------------

#ifndef _D3SDK

#ifdef GAME_DLL

#include "../game/Game_local.h"

#else

#include "../framework/DemoChecksum.h"

// framework
#include "../framework/Compressor.h"
#include "../framework/EventLoop.h"
#include "../framework/KeyInput.h"
#include "../framework/EditField.h"
#include "../framework/Console.h"
#include "../framework/DemoFile.h"
#include "../framework/Session.h"
//HUMANHEAD rww
#if _HH_SECUROM_DONOTREALLYNEED
#include "../framework/securom/securom_api.h"
#endif
//HUMANHEAD END

// asynchronous networking
#include "../framework/async/AsyncNetwork.h"

// The editor entry points are always declared, but may just be
// stubbed out on non-windows platforms.
#include "../tools/edit_public.h"

// Compilers for map, model, video etc. processing.
#include "../tools/compilers/compiler_public.h"

#endif /* !GAME_DLL */

#endif /* !_D3SDK */

//-----------------------------------------------------


#endif /* !__PRECOMPILED_H__ */
