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

#ifndef __MASTERSERVERLIST_H__
#define __MASTERSERVERLIST_H__

class idMasterServer {
public:
	idMasterServer() {}

	void SetName( const char *_name ) { name = idStr(_name); }
	idStr GetName() { return name; }

	void SetAddress( const char *_address ) { server = idStr(_address); }
	idStr GetAddress() { return server; }

	void SetPorts( const char *_port )	{ port = idStr(_port); }
	idStr GetPorts()	{ return port; }

private:
	idStr name;
	idStr server;
	idStr port;
};

class idMasterServerDecl {
public:
	virtual bool LoadFile( const char *fileName, bool OSPath = false );
	virtual bool FindServer( const char *name, idMasterServer **server );
	virtual idMasterServer* GetServerByIndex( int index );
	virtual int GetServerCount( void );
	virtual void Clear( void );

private:
	bool ReadList( idLexer &src, idMasterServer *entry );
	idList<idMasterServer *> masterservers;
};

#endif // !__MASTERSERVERLIST_H__
