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

#ifndef __DECLMODELDEF_H__
#define __DECLMODELDEF_H__

/*
==============================================================================================

	idDeclModelDef

==============================================================================================
*/

class idDeclModelDef : public idDecl {
public:
								idDeclModelDef();
								~idDeclModelDef();

	virtual size_t				Size( void ) const;
	virtual const char *		DefaultDefinition( void ) const;
	virtual bool				Parse( const char *text, const int textLength );
	virtual void				FreeData( void );

	void						Touch( void ) const;

	const idDeclSkin *			GetDefaultSkin( void ) const;
	const idJointQuat *			GetDefaultPose( void ) const;
	void						SetupJoints( int *numJoints, idJointMat **jointList, idBounds &frameBounds, bool removeOriginOffset ) const;
	idRenderModel *				ModelHandle( void ) const;
	void						GetJointList( const char *jointnames, idList<jointHandle_t> &jointList ) const;
	const jointInfo_t *			FindJoint( const char *name ) const;

	int							NumAnims( void ) const;
	const idAnim *				GetAnim( int index ) const;
	int							GetSpecificAnim( const char *name ) const;
	int							GetAnim( const char *name ) const;
	bool						HasAnim( const char *name ) const;
	const idDeclSkin *			GetSkin( void ) const;
	const char *				GetModelName( void ) const;
	const idList<jointInfo_t> &	Joints( void ) const;
	const int *					JointParents( void ) const;
	int							NumJoints( void ) const;
	const jointInfo_t *			GetJoint( int jointHandle ) const;
	const char *				GetJointName( int jointHandle ) const;
	int							NumJointsOnChannel( int channel ) const;
	const int *					GetChannelJoints( int channel ) const;

	const idVec3 &				GetVisualOffset( void ) const;

	// HUMANHEAD nla
	idDict						channelDict;
	// HUMANHEAD END

private:
	void						CopyDecl( const idDeclModelDef *decl );
	bool						ParseAnim( idLexer &src, int numDefaultAnims );

private:
	idVec3						offset;
	idList<jointInfo_t>			joints;
	idList<int>					jointParents;
	idList<int>					channelJoints[ ANIM_NumAnimChannels ];
	idRenderModel *				modelHandle;
	idList<idAnim *>			anims;
	const idDeclSkin *			skin;
};

#endif /* !__DECLSKIN_H__ */
