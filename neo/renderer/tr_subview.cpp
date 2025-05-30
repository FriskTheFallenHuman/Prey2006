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

#include "tr_local.h"


typedef struct {
	idVec3		origin;
	idMat3		axis;
} orientation_t;


/*
=================
R_MirrorPoint
=================
*/
static void R_MirrorPoint( const idVec3 in, orientation_t *surface, orientation_t *camera, idVec3 &out ) {
	int		i;
	idVec3	local;
	idVec3	transformed;
	float	d;

	local = in - surface->origin;

	transformed = vec3_origin;
	for ( i = 0 ; i < 3 ; i++ ) {
		d = local * surface->axis[i];
		transformed += d * camera->axis[i];
	}

	out = transformed + camera->origin;
}

/*
=================
R_MirrorVector
=================
*/
static void R_MirrorVector( const idVec3 in, orientation_t *surface, orientation_t *camera, idVec3 &out ) {
	int		i;
	float	d;

	out = vec3_origin;
	for ( i = 0 ; i < 3 ; i++ ) {
		d = in * surface->axis[i];
		out += d * camera->axis[i];
	}
}

/*
=============
R_PlaneForSurface

Returns the plane for the first triangle in the surface
FIXME: check for degenerate triangle?
=============
*/
static void R_PlaneForSurface( const srfTriangles_t *tri, idPlane &plane ) {
	idDrawVert		*v1, *v2, *v3;

	v1 = tri->verts + tri->indexes[0];
	v2 = tri->verts + tri->indexes[1];
	v3 = tri->verts + tri->indexes[2];
	plane.FromPoints( v1->xyz, v2->xyz, v3->xyz );
}

/*
=========================
R_PreciseCullSurface

Check the surface for visibility on a per-triangle basis
for cases when it is going to be VERY expensive to draw (subviews)

If not culled, also returns the bounding box of the surface in
Normalized Device Coordinates, so it can be used to crop the scissor rect.

OPTIMIZE: we could also take exact portal passing into consideration
=========================
*/
bool R_PreciseCullSurface( const drawSurf_t *drawSurf, idBounds &ndcBounds ) {
	const srfTriangles_t *tri;
	idPlane clip, eye;
	int i, j;
	unsigned int pointOr;
	unsigned int pointAnd;
	idVec3 localView;
	idFixedWinding w;

	tri = drawSurf->geo;

	pointOr = 0;
	pointAnd = (unsigned int)~0;

	// get an exact bounds of the triangles for scissor cropping
	ndcBounds.Clear();

	for ( i = 0; i < tri->numVerts; i++ ) {
		int j;
		unsigned int pointFlags;

		R_TransformModelToClip( tri->verts[i].xyz, drawSurf->space->modelViewMatrix,
			tr.viewDef->projectionMatrix, eye, clip );

		pointFlags = 0;
		for ( j = 0; j < 3; j++ ) {
			if ( clip[j] >= clip[3] ) {
				pointFlags |= (1 << (j*2));
			} else if ( clip[j] <= -clip[3] ) {
				pointFlags |= ( 1 << (j*2+1));
			}
		}

		pointAnd &= pointFlags;
		pointOr |= pointFlags;
	}

	// trivially reject
	if ( pointAnd ) {
		return true;
	}

	// backface and frustum cull
	R_GlobalPointToLocal( drawSurf->space->modelMatrix, tr.viewDef->renderView.vieworg, localView );

	for ( i = 0; i < tri->numIndexes; i += 3 ) {
		idVec3	dir, normal;
		float	dot;
		idVec3	d1, d2;

		const idVec3 &v1 = tri->verts[tri->indexes[i]].xyz;
		const idVec3 &v2 = tri->verts[tri->indexes[i+1]].xyz;
		const idVec3 &v3 = tri->verts[tri->indexes[i+2]].xyz;

		// this is a hack, because R_GlobalPointToLocal doesn't work with the non-normalized
		// axis that we get from the gui view transform.  It doesn't hurt anything, because
		// we know that all gui generated surfaces are front facing
		if ( tr.guiRecursionLevel == 0 ) {
			// we don't care that it isn't normalized,
			// all we want is the sign
			d1 = v2 - v1;
			d2 = v3 - v1;
			normal = d2.Cross( d1 );

			dir = v1 - localView;

			dot = normal * dir;
			if ( dot >= 0.0f ) {
				return true;
			}
		}

		// now find the exact screen bounds of the clipped triangle
		w.SetNumPoints( 3 );
		R_LocalPointToGlobal( drawSurf->space->modelMatrix, v1, w[0].ToVec3() );
		R_LocalPointToGlobal( drawSurf->space->modelMatrix, v2, w[1].ToVec3() );
		R_LocalPointToGlobal( drawSurf->space->modelMatrix, v3, w[2].ToVec3() );
		w[0].s = w[0].t = w[1].s = w[1].t = w[2].s = w[2].t = 0.0f;

		for ( j = 0; j < 4; j++ ) {
			if ( !w.ClipInPlace( -tr.viewDef->frustum[j], 0.1f ) ) {
				break;
			}
		}
		for ( j = 0; j < w.GetNumPoints(); j++ ) {
			idVec3	screen;

			R_GlobalToNormalizedDeviceCoordinates( w[j].ToVec3(), screen );
			ndcBounds.AddPoint( screen );
		}
	}

	// if we don't enclose any area, return
	if ( ndcBounds.IsCleared() ) {
		return true;
	}

	return false;
}

/*
========================
R_MirrorViewBySurface
========================
*/
static viewDef_t *R_MirrorViewBySurface( drawSurf_t *drawSurf ) {
	viewDef_t		*parms;
	orientation_t	surface, camera;
	idPlane			originalPlane, plane;

	// copy the viewport size from the original
	parms = (viewDef_t *)R_FrameAlloc( sizeof( *parms ) );
	*parms = *tr.viewDef;
	parms->renderView.viewID = 0;	// clear to allow player bodies to show up, and suppress view weapons

	parms->isSubview = true;
	parms->isMirror = true;

	// create plane axis for the portal we are seeing
	R_PlaneForSurface( drawSurf->geo, originalPlane );
	R_LocalPlaneToGlobal( drawSurf->space->modelMatrix, originalPlane, plane );

	surface.origin = plane.Normal() * -plane[3];
	surface.axis[0] = plane.Normal();
	surface.axis[0].NormalVectors( surface.axis[1], surface.axis[2] );
	surface.axis[2] = -surface.axis[2];

	camera.origin = surface.origin;
	camera.axis[0] = -surface.axis[0];
	camera.axis[1] = surface.axis[1];
	camera.axis[2] = surface.axis[2];

	// set the mirrored origin and axis
	R_MirrorPoint( tr.viewDef->renderView.vieworg, &surface, &camera, parms->renderView.vieworg );

	R_MirrorVector( tr.viewDef->renderView.viewaxis[0], &surface, &camera, parms->renderView.viewaxis[0] );
	R_MirrorVector( tr.viewDef->renderView.viewaxis[1], &surface, &camera, parms->renderView.viewaxis[1] );
	R_MirrorVector( tr.viewDef->renderView.viewaxis[2], &surface, &camera, parms->renderView.viewaxis[2] );

	// make the view origin 16 units away from the center of the surface
	idVec3	viewOrigin = ( drawSurf->geo->bounds[0] + drawSurf->geo->bounds[1] ) * 0.5;
	viewOrigin += ( originalPlane.Normal() * 16 );

	R_LocalPointToGlobal( drawSurf->space->modelMatrix, viewOrigin, parms->initialViewAreaOrigin );

	// set the mirror clip plane
	parms->numClipPlanes = 1;
	parms->clipPlanes[0] = -camera.axis[0];

	parms->clipPlanes[0][3] = -( camera.origin * parms->clipPlanes[0].Normal() );

	return parms;
}

/*
========================
R_XrayViewBySurface
========================
*/
static viewDef_t *R_XrayViewBySurface( drawSurf_t *drawSurf ) {
	viewDef_t		*parms;
	idPlane			originalPlane, plane;

	// copy the viewport size from the original
	parms = (viewDef_t *)R_FrameAlloc( sizeof( *parms ) );
	*parms = *tr.viewDef;
	parms->renderView.viewID = 0;	// clear to allow player bodies to show up, and suppress view weapons

	parms->isSubview = true;
	parms->isXraySubview = true;

	return parms;
}

/*
===============
R_RemoteRender
===============
*/
static void R_RemoteRender( drawSurf_t *surf, textureStage_t *stage ) {
	viewDef_t		*parms;

	// remote views can be reused in a single frame
	if ( stage->dynamicFrameCount == tr.frameCount ) {
		return;
	}

	// if the entity doesn't have a remoteRenderView, do nothing
	if ( !surf->space->entityDef->parms.remoteRenderView ) {
		return;
	}

	// copy the viewport size from the original
	parms = (viewDef_t *)R_FrameAlloc( sizeof( *parms ) );
	*parms = *tr.viewDef;

	parms->isSubview = true;
	parms->isMirror = false;

	parms->renderView = *surf->space->entityDef->parms.remoteRenderView;
	parms->renderView.viewID = 0;	// clear to allow player bodies to show up, and suppress view weapons
	parms->initialViewAreaOrigin = parms->renderView.vieworg;

	tr.CropRenderSize( stage->width, stage->height, true );

	parms->renderView.x = 0;
	parms->renderView.y = 0;
	parms->renderView.width = SCREEN_WIDTH;
	parms->renderView.height = SCREEN_HEIGHT;

	tr.RenderViewToViewport( &parms->renderView, &parms->viewport );

	parms->scissor.x1 = 0;
	parms->scissor.y1 = 0;
	parms->scissor.x2 = parms->viewport.x2 - parms->viewport.x1;
	parms->scissor.y2 = parms->viewport.y2 - parms->viewport.y1;

	parms->superView = tr.viewDef;
	parms->subviewSurface = surf;

	// generate render commands for it
	R_RenderView(parms);

	// copy this rendering to the image
	stage->dynamicFrameCount = tr.frameCount;
	if (!stage->image) {
		stage->image = globalImages->scratchImage;
	}

	tr.CaptureRenderToImage( stage->image->imgName );
	tr.UnCrop();
}

/*
=================
R_MirrorRender
=================
*/
void R_MirrorRender( drawSurf_t *surf, textureStage_t *stage, idScreenRect scissor ) {
	viewDef_t		*parms;

	// remote views can be reused in a single frame
	if ( stage->dynamicFrameCount == tr.frameCount ) {
		return;
	}

	// issue a new view command
	parms = R_MirrorViewBySurface( surf );
	if ( !parms ) {
		return;
	}

	tr.CropRenderSize( stage->width, stage->height, true );

	parms->renderView.x = 0;
	parms->renderView.y = 0;
	parms->renderView.width = SCREEN_WIDTH;
	parms->renderView.height = SCREEN_HEIGHT;

	tr.RenderViewToViewport( &parms->renderView, &parms->viewport );

	parms->scissor.x1 = 0;
	parms->scissor.y1 = 0;
	parms->scissor.x2 = parms->viewport.x2 - parms->viewport.x1;
	parms->scissor.y2 = parms->viewport.y2 - parms->viewport.y1;

	parms->superView = tr.viewDef;
	parms->subviewSurface = surf;

	// triangle culling order changes with mirroring
	parms->isMirror = ( ( (int)parms->isMirror ^ (int)tr.viewDef->isMirror ) != 0 );

	// generate render commands for it
	R_RenderView( parms );

	// copy this rendering to the image
	stage->dynamicFrameCount = tr.frameCount;
	stage->image = globalImages->scratchImage;

	tr.CaptureRenderToImage( stage->image->imgName );
	tr.UnCrop();
}

/*
=================
R_XrayRender
=================
*/
void R_XrayRender( drawSurf_t *surf, textureStage_t *stage, idScreenRect scissor ) {
	viewDef_t		*parms;

	// remote views can be reused in a single frame
	if ( stage->dynamicFrameCount == tr.frameCount ) {
		return;
	}

	// issue a new view command
	parms = R_XrayViewBySurface( surf );
	if ( !parms ) {
		return;
	}

	tr.CropRenderSize( stage->width, stage->height, true );

	parms->renderView.x = 0;
	parms->renderView.y = 0;
	parms->renderView.width = SCREEN_WIDTH;
	parms->renderView.height = SCREEN_HEIGHT;

	tr.RenderViewToViewport( &parms->renderView, &parms->viewport );

	parms->scissor.x1 = 0;
	parms->scissor.y1 = 0;
	parms->scissor.x2 = parms->viewport.x2 - parms->viewport.x1;
	parms->scissor.y2 = parms->viewport.y2 - parms->viewport.y1;

	parms->superView = tr.viewDef;
	parms->subviewSurface = surf;

	// triangle culling order changes with mirroring
	parms->isMirror = ( ( (int)parms->isMirror ^ (int)tr.viewDef->isMirror ) != 0 );

	// generate render commands for it
	R_RenderView( parms );

	// copy this rendering to the image
	stage->dynamicFrameCount = tr.frameCount;
	stage->image = globalImages->scratchImage2;

	tr.CaptureRenderToImage( stage->image->imgName );
	tr.UnCrop();
}

/*
==================
R_GenerateSurfaceSubview
==================
*/
bool	R_GenerateSurfaceSubview( drawSurf_t *drawSurf ) {
	idBounds		ndcBounds;
	viewDef_t		*parms;
	const idMaterial		*shader;

	// for testing the performance hit
	if ( r_skipSubviews.GetBool() ) {
		return false;
	}

	if ( R_PreciseCullSurface( drawSurf, ndcBounds ) ) {
		return false;
	}

	shader = drawSurf->material;

	// never recurse through a subview surface that we are
	// already seeing through
	for ( parms = tr.viewDef ; parms ; parms = parms->superView ) {
		if ( parms->subviewSurface
			&& parms->subviewSurface->geo == drawSurf->geo
			&& parms->subviewSurface->space->entityDef == drawSurf->space->entityDef ) {
			break;
		}
	}
	if ( parms ) {
		return false;
	}

	// crop the scissor bounds based on the precise cull
	idScreenRect	scissor;

	idScreenRect	*v = &tr.viewDef->viewport;
	scissor.x1 = v->x1 + (int)( (v->x2 - v->x1 + 1 ) * 0.5f * ( ndcBounds[0][0] + 1.0f ));
	scissor.y1 = v->y1 + (int)( (v->y2 - v->y1 + 1 ) * 0.5f * ( ndcBounds[0][1] + 1.0f ));
	scissor.x2 = v->x1 + (int)( (v->x2 - v->x1 + 1 ) * 0.5f * ( ndcBounds[1][0] + 1.0f ));
	scissor.y2 = v->y1 + (int)( (v->y2 - v->y1 + 1 ) * 0.5f * ( ndcBounds[1][1] + 1.0f ));

	// nudge a bit for safety
	scissor.Expand();

	scissor.Intersect( tr.viewDef->scissor );

	if ( scissor.IsEmpty() ) {
		// cropped out
		return false;
	}

	// DG: r_lockSurfaces needs special treatment
	if ( r_lockSurfaces.GetBool() && tr.viewDef == tr.primaryView ) {
		// we need the scissor for the "real" viewDef (actual camera position etc)
		// so mirrors don't "float around" when looking around with r_lockSurfaces enabled
		// So do the same calculation as before, but with real viewDef (but don't replace
		// calculation above, so the whole mirror or whatever is skipped if not visible in
		// locked view!)
		viewDef_t* origViewDef = tr.viewDef;
		tr.viewDef = &tr.lockSurfacesRealViewDef;
		R_PreciseCullSurface( drawSurf, ndcBounds );

		idScreenRect origScissor = scissor;

		idScreenRect	*v2 = &tr.viewDef->viewport;
		scissor.x1 = v2->x1 + (int)( (v2->x2 - v2->x1 + 1 ) * 0.5f * ( ndcBounds[0][0] + 1.0f ));
		scissor.y1 = v2->y1 + (int)( (v2->y2 - v2->y1 + 1 ) * 0.5f * ( ndcBounds[0][1] + 1.0f ));
		scissor.x2 = v2->x1 + (int)( (v2->x2 - v2->x1 + 1 ) * 0.5f * ( ndcBounds[1][0] + 1.0f ));
		scissor.y2 = v2->y1 + (int)( (v2->y2 - v2->y1 + 1 ) * 0.5f * ( ndcBounds[1][1] + 1.0f ));

		// nudge a bit for safety
		scissor.Expand();

		scissor.Intersect( tr.viewDef->scissor );

		// TBH I'm not 100% happy with how this is handled - you won't get reliable information
		// on what's rendered in a mirror this way. Intersecting with the orig. scissor looks "best".
		// For handling this "properly" we'd need the whole "locked viewDef vs real viewDef" thing
		// for every subview (instead of just once for the primaryView) which would be a lot of
		// work for a corner case...
		scissor.Intersect( origScissor );
		tr.viewDef = origViewDef;
	} // DG end

	// see what kind of subview we are making
	if ( shader->GetSort() != SS_SUBVIEW ) {
		for ( int i = 0 ; i < shader->GetNumStages() ; i++ ) {
			const shaderStage_t	*stage = shader->GetStage( i );
			switch ( stage->texture.dynamic ) {
			case DI_REMOTE_RENDER:
				R_RemoteRender( drawSurf, const_cast<textureStage_t *>(&stage->texture) );
				break;
			case DI_MIRROR_RENDER:
				R_MirrorRender( drawSurf, const_cast<textureStage_t *>(&stage->texture), scissor );
				break;
			case DI_XRAY_RENDER:
				R_XrayRender( drawSurf, const_cast<textureStage_t *>(&stage->texture), scissor );
				break;
			}
		}
		return true;
	}

	switch ( shader->GetSubviewClass() ) {
		case SC_PORTAL: {
			if ( !drawSurf->space->entityDef->parms.remoteRenderView ) {
				return false;
			}
			// if( tr.viewDef->isSubview ) return false;

			//k: idMaterial::directPortalDistance may be max distance for render, also see `materials/portals.mtr`.
			int index = shader->GetDirectPortalDistance();
			if ( index >= 0 && index < EXP_REG_NUM_PREDEFINED ) {
				float maxPortalDistanceLimit = drawSurf->space->entityDef->parms.shaderParms[index];
				//k: maybe parm == 0
				if ( maxPortalDistanceLimit > 0.0f && ( tr.viewDef->renderView.vieworg - drawSurf->space->entityDef->parms.origin ).LengthFast() > maxPortalDistanceLimit ) {
					return false;
				}
			}

			// copy the viewport size from the original
			parms = (viewDef_t *)R_FrameAlloc( sizeof(*parms) );
			if ( !parms ) {
				return false;
			}

			*parms = *tr.viewDef;

			parms->isSubview = true;
			parms->isMirror = false;

			const renderView_t *remoteRenderView = drawSurf->space->entityDef->parms.remoteRenderView;
			parms->renderView.viewID = 0;	// clear to allow player bodies to show up, and suppress view weapons

			idVec3 forward, left, up, forward2, left2, up2;
			idVec3 pos, pos2;
			const float *dsm = drawSurf->space->modelMatrix;
			float mm[16] = { //karin: inverse forward and right
				-dsm[0], -dsm[1], -dsm[2], dsm[3],
				-dsm[4], -dsm[5], -dsm[6], dsm[7],
				dsm[8], dsm[9], dsm[10], dsm[11],
				dsm[12], dsm[13], dsm[14], dsm[15],
			};

			//k: add a clip plane in remote camera
			parms->numClipPlanes = 1;
			parms->clipPlanes[0] = remoteRenderView->viewaxis[0];
			parms->clipPlanes[0][3] = -( remoteRenderView->vieworg * parms->clipPlanes[0].Normal() );

			//k: transform current render view origin and axis to surface model coordonate system
			R_GlobalVectorToLocal( mm, tr.viewDef->renderView.viewaxis[0], forward );
			R_GlobalVectorToLocal( mm, tr.viewDef->renderView.viewaxis[1], left );
			R_GlobalVectorToLocal( mm, tr.viewDef->renderView.viewaxis[2], up );
			R_GlobalPointToLocal( mm, tr.viewDef->renderView.vieworg, pos );

			//k: transform local origin and axis to remote view coordonate system
			float mmm[16];
			R_AxisToModelMatrix( remoteRenderView->viewaxis, remoteRenderView->vieworg, mmm );
			R_LocalVectorToGlobal( mmm, forward, forward2 );
			R_LocalVectorToGlobal( mmm, left, left2 );
			R_LocalVectorToGlobal( mmm, up, up2 );
			R_LocalPointToGlobal( mmm, pos, pos2 );

			//k: setup remote view origin and axis
			idMat3 hh3( forward2, left2, up2 );
			parms->renderView.viewaxis = hh3;
			parms->initialViewAreaOrigin = remoteRenderView->vieworg + remoteRenderView->viewaxis[0] * 16; //k: offset TODO how many???, this value will be using find areaNum
			parms->renderView.vieworg = pos2;

			parms->superView = tr.viewDef;
			parms->subviewSurface = drawSurf;

			// generate render commands for it
			R_RenderView( parms );
			return true;
		}
		case SC_PORTAL_SKYBOX: {
			if ( !drawSurf->space->entityDef->parms.remoteRenderView ) {
				return false;
			}

			//lvonasek: Allow max 1 skybox per frame
			if ( tr.SkyboxRenderedInFrame() || tr.viewDef->isSubview ) {
				return false;
			}

			// copy the viewport size from the original
			parms = (viewDef_t *)R_FrameAlloc( sizeof(*parms) );
			if ( !parms ) {
				return false;
			}
			tr.RenderSkyboxInFrame();
			*parms = *tr.viewDef;

			parms->isSubview = true;
			parms->isMirror = false;

			const renderView_t *remoteRenderView = drawSurf->space->entityDef->parms.remoteRenderView;
			parms->renderView.viewID = 0;	// clear to allow player bodies to show up, and suppress view weapons
			parms->initialViewAreaOrigin = remoteRenderView->vieworg;
			parms->renderView.vieworg = remoteRenderView->vieworg;

			parms->superView = tr.viewDef;
			parms->subviewSurface = drawSurf;

			// generate render commands for it
			R_RenderView( parms );
			return true;
		}
		case SC_MIRROR:
		default: {
			// issue a new view command
			parms = R_MirrorViewBySurface( drawSurf );
			if ( !parms ) {
				return false;
			}

			parms->scissor = scissor;
			parms->superView = tr.viewDef;
			parms->subviewSurface = drawSurf;

			// triangle culling order changes with mirroring
			parms->isMirror = ( ( (int)parms->isMirror ^ (int)tr.viewDef->isMirror ) != 0 );

			// generate render commands for it
			R_RenderView( parms );

			return true;
		}
	}
}

/*
================
R_GenerateSubViews

If we need to render another view to complete the current view,
generate it first.

It is important to do this after all drawSurfs for the current
view have been generated, because it may create a subview which
would change tr.viewCount.
================
*/
bool R_GenerateSubViews( void ) {
	drawSurf_t		*drawSurf;
	int				i;
	bool			subviews;
	const idMaterial		*shader;

	// for testing the performance hit
	if ( r_skipSubviews.GetBool() ) {
		return false;
	}

	subviews = false;

	// scan the surfaces until we either find a subview, or determine
	// there are no more subview surfaces.
	for ( i = 0 ; i < tr.viewDef->numDrawSurfs ; i++ ) {
		drawSurf = tr.viewDef->drawSurfs[i];
		shader = drawSurf->material;

		if ( !shader || !shader->HasSubview() ) {
			continue;
		}

		if ( R_GenerateSurfaceSubview( drawSurf ) ) {
			subviews = true;
		}
	}

	return subviews;
}
