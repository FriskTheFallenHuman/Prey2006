/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 BFG Edition GPL Source Code ("Doom 3 BFG Edition Source Code").

Doom 3 BFG Edition Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 BFG Edition Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 BFG Edition Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 BFG Edition Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#ifndef __MODELOVERLAY_H__
#define __MODELOVERLAY_H__

/*
===============================================================================

	Overlays are used for adding decals on top of dynamic models.
	Projects an overlay onto deformable geometry and can be added to
	a render entity to allow decals on top of dynamic models.
	This does not generate tangent vectors, so it can't be used with
	light interaction shaders. Materials for overlays should always
	be clamped, because the projected texcoords can run well off the
	texture since no new clip vertexes are generated.
	Overlays with common materials will be merged together, but additional
	overlays will be allocated as needed. The material should not be
	one that receives lighting, because no interactions are generated
	for these lightweight surfaces.

===============================================================================
*/

static const int MAX_OVERLAYS_PER_MATERIAL = 16;
static const int OVERLAYS_GRANULARITY = 4;
static const int DEFERRED_OVERLAYS_GRANULARITY = 8;

struct overlayProjectionParms_t {
	idPlane				localTextureAxis[2];
	const idMaterial *	material;
	int					startTime;
};

struct overlayVertex_t {
	int					vertexNum;
	halfFloat_t			st[2];
};

struct overlay_t {
	int					surfaceNum;
	int					surfaceId;
	int					maxReferencedVertex;
	int					numIndexes;
	triIndex_t *		indexes;
	int					numVerts;
	overlayVertex_t *	verts;
};

struct overlaySurfaces_t {
	overlaySurfaces_t() :
		material( NULL ),
		surfaces( MAX_OVERLAYS_PER_MATERIAL ) {

	}

	const idMaterial *				material;
	idList<overlay_t, TAG_MODEL>	surfaces;
};

class idRenderModelOverlay {
public:
								idRenderModelOverlay();
								~idRenderModelOverlay();

	void						AddDeferredOverlay( const overlayProjectionParms_t & localParms );
	void						CreateDeferredOverlays( const idRenderModel * model );

	unsigned int				GetNumOverlayDrawSurfs();
	struct drawSurf_t *			CreateOverlayDrawSurf( const struct viewEntity_t *space, const idRenderModel *baseModel, unsigned int index );

	void						ReadFromDemoFile( class idDemoFile *f );
	void						WriteToDemoFile( class idDemoFile *f ) const;

private:
	idList<overlaySurfaces_t, TAG_MODEL>			overlays;
	idList<overlayProjectionParms_t, TAG_MODEL>		deferredOverlays;

	void						CreateOverlay( const idRenderModel *model, const idPlane localTextureAxis[2], const idMaterial *material );
	void						FreeOverlay( overlay_t & overlay );
};

#endif /* !__MODELOVERLAY_H__ */
