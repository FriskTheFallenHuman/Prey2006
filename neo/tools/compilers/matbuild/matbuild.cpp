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

#include "precompiled.h"
#pragma hdrstop

/*
	builds interaction materials for a given directory.
*/

/*
================
MatBuildDir_f
================
*/
void MatBuildDir_f( const idCmdArgs& args )
{
	com_editorCMDActive = true;

	if( args.Argc() < 2 )
	{
		common->Printf( "Usage: matbuilddir <folder>\n" );
		com_editorCMDActive = false;
		return;
	}

	idFileList* fileList = fileSystem->ListFiles( va( "textures/%s", args.Argv( 1 ) ), "" );
	idStr		mtrBuffer;

	idStrList	list = fileList->GetList();

	mtrBuffer += va( "// NOTE:  THIS FILE IS WAS GENERATE BY THE ENGINE\n" );
	mtrBuffer += va( "// MANUAL EDITING MAY BE REQUIERED\n" );

	for( int i = 0; i < fileList->GetNumFiles(); i++ )
	{
		idStr imagepath = list[i];

		// Normal maps
		if( strstr( imagepath.c_str(), "_local" ) )
		{
			continue;
		}

		// Specular
		if( strstr( imagepath.c_str(), "_spec" ) )
		{
			continue;
		}

		// Height map
		if( strstr( imagepath.c_str(), "_h" ) )
		{
			continue;
		}

		imagepath = imagepath.StripFileExtension();
		imagepath.Strip( "_d" );

		mtrBuffer += va( "\n" );
		mtrBuffer += va( "textures/%s/%s\n", args.Argv( 1 ), imagepath.c_str() );
		mtrBuffer += va( "{\n" );
		mtrBuffer += va( "\tqer_editorimage textures/%s/%s_d.%s\n", args.Argv( 1 ), args.Argv( 2 ), imagepath.c_str() );
		mtrBuffer += va( "\tdiffusemap textures/%s/%s_d\n", args.Argv( 1 ), imagepath.c_str() );
		mtrBuffer += va( "\tbumpmap addnormals ( textures/%s/%s_local, heightmap ( textures/%s/%s_h, 4 ) )\n", args.Argv( 1 ), imagepath.c_str(), args.Argv( 1 ), imagepath.c_str() );
		mtrBuffer += va( "\tspecularmap textures/%s/%s_spec\n", args.Argv( 1 ), imagepath.c_str() );
		mtrBuffer += va( "}\n" );
	}

	fileSystem->FreeFileList( fileList );

	idStr mtrName = args.Argv( 1 );
	fileSystem->WriteFile( va( "materials/%s.mtr", mtrName.c_str() ), mtrBuffer.c_str(), mtrBuffer.Length(), "fs_basepath" );

	com_editorCMDActive = false;
}
