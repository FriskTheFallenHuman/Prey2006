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

idMikkTSpaceInterface mikkTSpaceInterface;

/*
============
SetUpMikkTSpaceContext
============
*/
static void SetUpMikkTSpaceContext( SMikkTSpaceContext *context ) {
	context->m_pInterface = &mikkTSpaceInterface.mkInterface;
}

/*
============
R_DeriveMikktspaceTangents

Derives the tangent space for the given triangles using the Mikktspace standard.
Normals must be calculated beforehand.
============
*/
bool R_DeriveMikktspaceTangents( srfTriangles_t *tri ) {

	SMikkTSpaceContext context;
	SetUpMikkTSpaceContext( &context );
	context.m_pUserData = tri;

	return ( genTangSpaceDefault( &context ) != 0 );

}

/*
============
mkAlloc
============
*/
static void *mkAlloc( int bytes ) {
	return R_StaticAlloc( bytes );
}

/*
============
mkFree
============
*/
static void mkFree( void *mem ) {
	R_StaticFree( mem );
}

/*
============
mkGetNumFaces
============
*/
static int mkGetNumFaces( const SMikkTSpaceContext *pContext ) {
	srfTriangles_t *tris = reinterpret_cast<srfTriangles_t *>( pContext->m_pUserData );
	return tris->numIndexes / 3;
}

/*
============
mkGetNumVerticesOfFace
============
*/
static int mkGetNumVerticesOfFace( const SMikkTSpaceContext *pContext, const int iFace ) {
	return 3;
}

/*
============
mkGetPosition
============
*/
static void mkGetPosition( const SMikkTSpaceContext *pContext, float fvPosOut[], const int iFace, const int iVert ) {
	srfTriangles_t *tris = reinterpret_cast<srfTriangles_t *>( pContext->m_pUserData );

	const int vertIndex = iFace * 3;
	const int index = tris->indexes[vertIndex + iVert];
	const idDrawVert& vert = tris->verts[index];

	fvPosOut[0] = vert.xyz[0];
	fvPosOut[1] = vert.xyz[1];
	fvPosOut[2] = vert.xyz[2];
}

/*
============
mkGetNormal
============
*/
static void mkGetNormal( const SMikkTSpaceContext *pContext, float fvNormOut[], const int iFace, const int iVert ) {
	srfTriangles_t *tris = reinterpret_cast<srfTriangles_t *>( pContext->m_pUserData );

	const int vertIndex = iFace * 3;
	const int index = tris->indexes[vertIndex + iVert];
	const idDrawVert& vert = tris->verts[index];

	idVec3 norm = vert.normal; norm.Normalize();
	fvNormOut[0] = norm.x;
	fvNormOut[1] = norm.y;
	fvNormOut[2] = norm.z;
}

/*
============
mkGetTexCoord
============
*/
static void mkGetTexCoord( const SMikkTSpaceContext *pContext, float fvTexcOut[], const int iFace, const int iVert ) {
	srfTriangles_t *tris = reinterpret_cast<srfTriangles_t *>( pContext->m_pUserData );

	const int vertIndex = iFace * 3;
	const int index = tris->indexes[vertIndex + iVert];
	const idDrawVert& vert = tris->verts[index];

	const idVec2 texCoord = vert.st;
	fvTexcOut[0] = texCoord.x;
	fvTexcOut[1] = texCoord.y;
}

/*
============
mkSetTSpaceBasic
============
*/
static void mkSetTSpaceBasic(const SMikkTSpaceContext *pContext, const float fvTangent[], const float fSign, const int iFace, const int iVert) {
	srfTriangles_t *tris = reinterpret_cast<srfTriangles_t *>( pContext->m_pUserData );

	const int vertIndex = iFace * 3;
	const int index = tris->indexes[vertIndex + iVert];

	const idVec3 tangent( fvTangent[0], fvTangent[1], fvTangent[2] );
//	tris->verts[index].SetTangent( tangent );
//	tris->verts[index].SetBiTangentSign( fSign );
	tris->verts[index].SetTangent( tangent, fSign );
}

/*
============
idMikkTSpaceInterface::idMikkTSpaceInterface
============
*/
idMikkTSpaceInterface::idMikkTSpaceInterface( void ) : mkInterface() {
	mkInterface.m_alloc = mkAlloc;
	mkInterface.m_free = mkFree;
	mkInterface.m_getNumFaces = mkGetNumFaces;
	mkInterface.m_getNumVerticesOfFace = mkGetNumVerticesOfFace;
	mkInterface.m_getPosition = mkGetPosition;
	mkInterface.m_getNormal = mkGetNormal;
	mkInterface.m_getTexCoord = mkGetTexCoord;
	mkInterface.m_setTSpaceBasic = mkSetTSpaceBasic;
}