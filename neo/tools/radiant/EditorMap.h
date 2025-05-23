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

extern	char		currentmap[1024];

// head/tail of doubly linked lists
extern	idEditorBrush	active_brushes;	// brushes currently being displayed
extern	idEditorBrush	selected_brushes;	// highlighted


extern CPtrArray& g_ptrSelectedFaces;
extern CPtrArray& g_ptrSelectedFaceBrushes;
//extern	face_t	*selected_face;
//extern	idEditorBrush	*selected_face_brush;
extern	idEditorBrush	filtered_brushes;	// brushes that have been filtered or regioned

extern	idEditorEntity	entities;
extern	idEditorEntity*	world_entity;	// the world entity is NOT included in
// the entities chain

extern	int modified;		// for quit confirmations

extern	idVec3	region_mins, region_maxs;
extern	bool	region_active;

void	Map_LoadFile( const char* filename );
bool	Map_SaveFile( const char* filename, bool use_region, bool autosave = false );
void	Map_New();
void	Map_BuildBrushData();

void	Map_RegionOff();
void	Map_RegionXY();
void	Map_RegionTallBrush();
void	Map_RegionBrush();
void	Map_RegionSelectedBrushes();
bool	Map_IsBrushFiltered( idEditorBrush* b );

void	Map_SaveSelected( CMemFile* pMemFile, CMemFile* pPatchFile = NULL );
void	Map_ImportBuffer( char* buf, bool renameEntities = true );
int		Map_GetUniqueEntityID( const char* prefix, const char* eclass );

idMapPrimitive* BrushToMapPrimitive( const idEditorBrush* b, const idVec3& origin );
