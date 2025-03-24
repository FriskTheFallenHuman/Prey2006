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

#include "renderer/tr_local.h"
#include "renderer/Model_local.h"

// Simple beam model. different with idRenderModelBeam, the line's start point is not view origin.

static const struct viewDef_s *current_view; // temp, should as a parameter

/*
====================
hhRenderModelBeam::InitFromFile
====================
*/
void hhRenderModelBeam::InitFromFile( const char *fileName ) {
	name = fileName;
	declBeam = static_cast<const hhDeclBeam *>( declManager->FindType( DECL_BEAM, fileName ) );
}

/*
====================
hhRenderModelBeam::LoadModel
====================
*/
void hhRenderModelBeam::LoadModel( void ) {
	idRenderModelStatic::LoadModel();
}

/*
====================
hhRenderModelBeam::IsDynamicModel
====================
*/
dynamicModel_t hhRenderModelBeam::IsDynamicModel( void ) const {
	return DM_CONTINUOUS;	// regenerate for every view
}

/*
====================
hhRenderModelBeam::InstantiateDynamicModel
====================
*/
idRenderModel *hhRenderModelBeam::InstantiateDynamicModel( const struct renderEntity_s *renderEntity, const struct viewDef_s *view, idRenderModel *cachedModel ) {
	idRenderModelStatic *staticModel;
	srfTriangles_t *tri;
	modelSurface_t surf;

	if ( cachedModel ) {
		delete cachedModel;
		cachedModel = NULL;
	}

	if ( renderEntity == NULL || view == NULL ) {
		delete cachedModel;
		return NULL;
	}

	if ( !declBeam || !renderEntity->beamNodes || declBeam->numNodes < 2 ) {
		return NULL;
	}

	if ( cachedModel != NULL ) {

		assert( dynamic_cast<idRenderModelStatic *>( cachedModel ) != NULL );
		staticModel = static_cast<idRenderModelStatic *>( cachedModel );
	} else {
		staticModel = new idRenderModelStatic;
		int id = 0;

		// line
		for( int i = 0; i < declBeam->numBeams; i++ ) {
			int vertNum = declBeam->numNodes - 1;
			tri = R_AllocStaticTriSurf();
			R_AllocStaticTriSurfVerts( tri, 4 * vertNum );
			R_AllocStaticTriSurfIndexes( tri, 6 * vertNum );

			for( int m = 0; m < vertNum; m++ ) {
				tri->verts[4 * m + 0].Clear();
				tri->verts[4 * m + 0].st[0] = 0;
				tri->verts[4 * m + 0].st[1] = 0;
				tri->verts[4 * m + 0].color[0] = 1.0;
				tri->verts[4 * m + 0].color[1] = 1.0;
				tri->verts[4 * m + 0].color[2] = 1.0;
				tri->verts[4 * m + 0].color[3] = 1.0;

				tri->verts[4 * m + 1].Clear();
				tri->verts[4 * m + 1].st[0] = 0;
				tri->verts[4 * m + 1].st[1] = 1;
				tri->verts[4 * m + 1].color[0] = 1.0;
				tri->verts[4 * m + 1].color[1] = 1.0;
				tri->verts[4 * m + 1].color[2] = 1.0;
				tri->verts[4 * m + 1].color[3] = 1.0;

				tri->verts[4 * m + 2].Clear();
				tri->verts[4 * m + 2].st[0] = 1;
				tri->verts[4 * m + 2].st[1] = 0;
				tri->verts[4 * m + 2].color[0] = 1.0;
				tri->verts[4 * m + 2].color[1] = 1.0;
				tri->verts[4 * m + 2].color[2] = 1.0;
				tri->verts[4 * m + 2].color[3] = 1.0;

				tri->verts[4 * m + 3].Clear();
				tri->verts[4 * m + 3].st[0] = 1;
				tri->verts[4 * m + 3].st[1] = 1;
				tri->verts[4 * m + 3].color[0] = 1.0;
				tri->verts[4 * m + 3].color[1] = 1.0;
				tri->verts[4 * m + 3].color[2] = 1.0;
				tri->verts[4 * m + 3].color[3] = 1.0;

				tri->indexes[6 * m + 0] = 4 * m + 0;
				tri->indexes[6 * m + 1] = 4 * m + 2;
				tri->indexes[6 * m + 2] = 4 * m + 1;
				tri->indexes[6 * m + 3] = 4 * m + 2;
				tri->indexes[6 * m + 4] = 4 * m + 3;
				tri->indexes[6 * m + 5] = 4 * m + 1;
			}

			tri->numVerts = 4 * vertNum;
			tri->numIndexes = 6 * vertNum;

			surf.geometry = tri;
			surf.id = id++;
//			surf.shader = tr.defaultMaterial;
			surf.shader = declBeam->shader[i];

			staticModel->AddSurface( surf );
		}

		// quad
		for( int i = 0; i < declBeam->numBeams; i++ ) {
			for( int m = 0; m < 2; m++ ) {
				// no material
				if( !declBeam->quadShader[i][m] ) {
					continue;
				}

				tri = R_AllocStaticTriSurf();
				R_AllocStaticTriSurfVerts( tri, 4 );
				R_AllocStaticTriSurfIndexes( tri, 6 );

				tri->verts[0].Clear();
				tri->verts[0].st[0] = 0;
				tri->verts[0].st[1] = 0;
				tri->verts[0].color[0] = 1.0;
				tri->verts[0].color[1] = 1.0;
				tri->verts[0].color[2] = 1.0;
				tri->verts[0].color[3] = 1.0;

				tri->verts[1].Clear();
				tri->verts[1].st[0] = 0;
				tri->verts[1].st[1] = 1;
				tri->verts[1].color[0] = 1.0;
				tri->verts[1].color[1] = 1.0;
				tri->verts[1].color[2] = 1.0;
				tri->verts[1].color[3] = 1.0;

				tri->verts[2].Clear();
				tri->verts[2].st[0] = 1;
				tri->verts[2].st[1] = 0;
				tri->verts[2].color[0] = 1.0;
				tri->verts[2].color[1] = 1.0;
				tri->verts[2].color[2] = 1.0;
				tri->verts[2].color[3] = 1.0;

				tri->verts[3].Clear();
				tri->verts[3].st[0] = 1;
				tri->verts[3].st[1] = 1;
				tri->verts[3].color[0] = 1.0;
				tri->verts[3].color[1] = 1.0;
				tri->verts[3].color[2] = 1.0;
				tri->verts[3].color[3] = 1.0;

				tri->indexes[0] = 0;
				tri->indexes[1] = 2;
				tri->indexes[2] = 1;
				tri->indexes[3] = 2;
				tri->indexes[4] = 3;
				tri->indexes[5] = 1;

				tri->numVerts = 4;
				tri->numIndexes = 6;

				surf.geometry = tri;
				surf.id = id++;
				//			surf.shader = tr.defaultMaterial;
				surf.shader = declBeam->quadShader[i][m];

				staticModel->AddSurface( surf );
			}
		}
	}

	current_view = view; // should as a parameter
	int id = 0;
	for( int i = 0; i < declBeam->numBeams; i++ ) {
		UpdateSurface( renderEntity, i, renderEntity->beamNodes, const_cast<modelSurface_t *>( staticModel->Surface( i ) ) );
		id++;
	}
	for( int i = 0; i < declBeam->numBeams; i++ ) {
		for( int m = 0; m < 2; m++ ) {
			if( !declBeam->quadShader[i][m] ) {
				continue;
			}

			UpdateQuadSurface( renderEntity, i, m, renderEntity->beamNodes, const_cast<modelSurface_t *>( staticModel->Surface( id++ ) ) );
		}
	}

	staticModel->bounds = Bounds( renderEntity );

	//LOGI("BOUND %s %s %s", Name(), staticModel->bounds[0].ToString(), staticModel->bounds[1].ToString())
	return staticModel;
}

/*
====================
hhRenderModelBeam::Bounds
====================
*/
idBounds hhRenderModelBeam::Bounds( const struct renderEntity_s *renderEntity ) const {
	idBounds	b;

	b.Zero();

	if ( !renderEntity || !renderEntity->beamNodes ) {
		b.ExpandSelf( 8.0f );
	} else {
		for( int i = 0; i < declBeam->numBeams; i++ ) {
			for( int m = 0; m < declBeam->numNodes; m++ ) {
				const idVec3	&target = renderEntity->beamNodes->nodes[m];
				idBounds tb;
				tb.Zero();
				tb.AddPoint( target );
				tb.ExpandSelf( declBeam->thickness[i] * 0.5 );
				b += tb;
			}
		}
	}

	return b;
}

/*
====================
hhRenderModelBeam::UpdateSurface
====================
*/
void hhRenderModelBeam::UpdateSurface( const struct renderEntity_s *renderEntity, const int index, const hhBeamNodes_t *beam, modelSurface_t *surf ) {
	srfTriangles_t *tri = surf->geometry;
	idVec3 up;
	renderEntity->axis.ProjectVector( current_view->renderView.viewaxis[2], up );

	int numLines = declBeam->numNodes - 1;
	for( int i = 0; i < numLines; i++ ) {
		const idVec3	&start = beam->nodes[i];
		const idVec3	&target = beam->nodes[i + 1];

		idVec3 minor = up * declBeam->thickness[index] * 0.5f;

		tri->verts[4 * i + 0].xyz = start - minor;
		tri->verts[4 * i + 1].xyz = start + minor;
		tri->verts[4 * i + 2].xyz = target - minor;
		tri->verts[4 * i + 3].xyz = target + minor;
	}

	R_BoundTriSurf( tri );
}

/*
====================
hhRenderModelBeam::UpdateQuadSurface
====================
*/
void hhRenderModelBeam::UpdateQuadSurface( const struct renderEntity_s *renderEntity, const int index, int quadIndex, const hhBeamNodes_t *beam, modelSurface_t *surf ) {
	srfTriangles_t *tri = surf->geometry;
	idVec3 up;
	idVec3 right;
	renderEntity->axis.ProjectVector( current_view->renderView.viewaxis[2], up );
	renderEntity->axis.ProjectVector( current_view->renderView.viewaxis[1], right );
	idVec3 target = quadIndex == 0 ? beam->nodes[0] : beam->nodes[declBeam->numNodes - 1];

	idVec3 upw = up * declBeam->quadSize[index][quadIndex] * 0.5f;
	idVec3 rightw = right * declBeam->quadSize[index][quadIndex] * 0.5f;

	tri->verts[0].xyz = target - rightw - upw;
	tri->verts[1].xyz = target - rightw + upw;
	tri->verts[2].xyz = target + rightw - upw;
	tri->verts[3].xyz = target + rightw + upw;

	R_BoundTriSurf( tri );
}