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

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU
General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#pragma once

/*
=================================
This file holds the Radiant functionaly
that's not tied to any UI
=================================
*/

#define ID_SHOW_MODELS ( WM_USER + 28477 )

const int RAD_SHIFT	  = 0x01;
const int RAD_ALT	  = 0x02;
const int RAD_CONTROL = 0x04;
const int RAD_PRESS	  = 0x08;

struct SCommandInfo
{
	char*		 m_strCommand;
	unsigned int m_nKey;
	unsigned int m_nModifiers;
	unsigned int m_nCommand;
};

struct SKeyInfo
{
	char*		 m_strName;
	unsigned int m_nVKKey;
};

extern int		 g_nCommandCount;
extern int		 g_nKeyCount;

extern HINSTANCE g_DoomInstance;
extern bool		 g_editorAlive;

/*
=================================
Radiant "Editor" Implementation
=================================
*/
void			 RadiantPrint( const char* text );
void			 RadiantShutdown( void );
void			 RadiantInit( void );
void			 RadiantRun( void );

bool			 SaveRegistryInfo( const char* pszName, void* pvBuf, long lSize );
bool			 LoadRegistryInfo( const char* pszName, void* pvBuf, long* plSize );

bool			 SaveWindowState( HWND hWnd, const char* pszName );
bool			 LoadWindowState( HWND hWnd, const char* pszName );

void			 Sys_UpdateStatusBar( void );
void			 Sys_Status( const char* psz, int part );
