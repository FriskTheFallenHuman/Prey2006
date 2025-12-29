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

// brush.h

struct idEditorEntity;

struct idEditorBrush
{
	idEditorBrush *		   prev, *next;	  // links in active/selected
	idEditorBrush *		   oprev, *onext; // links in entity
	idEditorBrush*		   list;		  // keep a handy link to the list its in
	idEditorEntity*		   owner;
	idVec3				   mins, maxs;

	idVec3				   lightCenter; // for moving the shading center of point lights
	idVec3				   lightRight;
	idVec3				   lightTarget;
	idVec3				   lightUp;
	idVec3				   lightRadius;
	idVec3				   lightOffset;
	idVec3				   lightColor;
	idVec3				   lightStart;
	idVec3				   lightEnd;
	bool				   pointLight;
	bool				   startEnd;
	int					   lightTexture;

	bool				   trackLightOrigin; // this brush is a special case light brush
	bool				   entityModel;

	face_t*				   brush_faces;

	//
	// curve brush extensions
	// all are derived from brush_faces
	bool				   hiddenBrush;
	bool				   forceWireFrame;
	bool				   forceVisibile;

	patchMesh_t*		   pPatch;
	idEditorEntity*		   pUndoOwner;

	int					   undoId;	// undo ID
	int					   redoId;	// redo ID
	int					   ownerId; // entityId of the owner entity for undo

	int					   numberId; // brush number

	idRenderModel*		   modelHandle;
	mutable idRenderModel* animSnapshotModel;

	// brush primitive only
	idDict				   epairs;
};

idEditorBrush* Brush_Alloc();
void		   Brush_Free( idEditorBrush* b, bool bRemoveNode = true );
int			   Brush_MemorySize( const idEditorBrush* brush );
void		   Brush_MakeSided( int sides );
void		   Brush_MakeSidedCone( int sides );
void		   Brush_Move( idEditorBrush* b, const idVec3 move, bool bSnap = true, bool updateOrigin = true );
int			   Brush_MoveVertex( idEditorBrush* b, const idVec3& vertex, const idVec3& delta, idVec3& end, bool bSnap );
void		   Brush_ResetFaceOriginals( idEditorBrush* b );
idEditorBrush* Brush_Parse( const idVec3 origin );
face_t*		   Brush_Ray( idVec3 origin, idVec3 dir, idEditorBrush* b, float* dist, bool testPrimitive = false );
int			   Brush_ToTris( idEditorBrush* brush, idTriList* tris, idMatList* mats, bool models, bool bmodel );
;
void		   Brush_RemoveFromList( idEditorBrush* b );
void		   Brush_AddToList( idEditorBrush* b, idEditorBrush* list );
void		   Brush_Build( idEditorBrush* b, bool bSnap = true, bool bMarkMap = true, bool bConvert = false, bool updateLights = true );
void		   Brush_BuildWindings( idEditorBrush* b, bool bSnap = true, bool keepOnPlaneWinding = false, bool updateLights = true, bool makeFacePlanes = true );
idEditorBrush* Brush_Clone( idEditorBrush* b );
idEditorBrush* Brush_FullClone( idEditorBrush* b );
idEditorBrush* Brush_Create( idVec3 mins, idVec3 maxs, texdef_t* texdef );
void		   Brush_Draw( const idEditorBrush* b, bool bSelected = false );
void		   Brush_DrawXY( idEditorBrush* b, ViewType nViewType, bool bSelected = false, bool ignoreViewType = false );
void		   Brush_SplitBrushByFace( idEditorBrush* in, face_t* f, idEditorBrush** front, idEditorBrush** back );
void		   Brush_SelectFaceForDragging( idEditorBrush* b, face_t* f, bool shear );
void		   Brush_SetTexture( idEditorBrush* b, texdef_t* texdef, brushprimit_texdef_t* brushprimit_texdef, bool bFitScale = false );
void		   Brush_SideSelect( idEditorBrush* b, idVec3 origin, idVec3 dir, bool shear );
void		   Brush_SnapToGrid( idEditorBrush* pb );
void		   Brush_Rotate( idEditorBrush* b, idVec3 vAngle, idVec3 vOrigin, bool bBuild = true );
void		   Brush_MakeSidedSphere( int sides );
void		   Brush_Write( idEditorBrush* b, FILE* f, const idVec3& origin, bool newFormat );
void		   Brush_Write( idEditorBrush* b, CMemFile* pMemFile, const idVec3& origin, bool NewFormat );
idWinding*	   Brush_MakeFaceWinding( idEditorBrush* b, face_t* face, bool keepOnPlaneWinding = false );
void		   Brush_SetTextureName( idEditorBrush* b, const char* name );
void		   Brush_Print( idEditorBrush* b );
void		   Brush_FitTexture( idEditorBrush* b, float height, float width );
void		   Brush_SetEpair( idEditorBrush* b, const char* pKey, const char* pValue );
const char*	   Brush_GetKeyValue( idEditorBrush* b, const char* pKey );
void		   Brush_RebuildBrush( idEditorBrush* b, idVec3 vMins, idVec3 vMaxs, bool patch = true );
void		   Brush_GetBounds( idEditorBrush* b, idBounds& bo );

face_t*		   Face_Alloc( void );
void		   Face_Free( face_t* f );
face_t*		   Face_Clone( face_t* f );
void		   Face_MakePlane( face_t* f );
void		   Face_Draw( face_t* face );
void		   Face_TextureVectors( face_t* f, float STfromXYZ[2][4] );
void		   Face_FitTexture( face_t* face, float height, float width );
void		   SetFaceTexdef( idEditorBrush* b, face_t* f, texdef_t* texdef, brushprimit_texdef_t* brushprimit_texdef, bool bFitScale = false );

int			   AddPlanept( idVec3* f );
