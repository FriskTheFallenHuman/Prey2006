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

#ifndef __GAME_EDIT_H__
#define __GAME_EDIT_H__


/*
===============================================================================

	Ingame cursor.

===============================================================================
*/

class idCursor3D : public idEntity {
public:
	CLASS_PROTOTYPE( idCursor3D );

							idCursor3D( void );
							~idCursor3D( void );

	void					Spawn( void );
	void					Present( void );
	void					Think( void );

	idForce_Drag			drag;
	idVec3					draggedPosition;
};


/*
===============================================================================

	Allows entities to be dragged through the world with physics.

===============================================================================
*/

class idDragEntity {
public:
							idDragEntity( void );
							~idDragEntity( void );

	void					Clear();
	void					Update( idPlayer *player );
	void					SetSelected( idEntity *ent );
	idEntity *				GetSelected( void ) const { return selected.GetEntity(); }
	void					DeleteSelected( void );
	void					BindSelected( void );
	void					UnbindSelected( void );

private:
	idEntityPtr<idEntity>	dragEnt;			// entity being dragged
	jointHandle_t			joint;				// joint being dragged
	int						id;					// id of body being dragged
	idVec3					localEntityPoint;	// dragged point in entity space
	idVec3					localPlayerPoint;	// dragged point in player space
	idStr					bodyName;			// name of the body being dragged
	idCursor3D *			cursor;				// cursor entity
	idEntityPtr<idEntity>	selected;			// last dragged entity

	void					StopDrag( void );
};


/*
===============================================================================

	Handles ingame entity editing.

===============================================================================
*/
typedef struct selectedTypeInfo_s {
	idTypeInfo *typeInfo;
	idStr		textKey;
} selectedTypeInfo_t;

class idEditEntities {
public:
							idEditEntities( void );
	bool					SelectEntity( const idVec3 &origin, const idVec3 &dir, const idEntity *skip );
	void					AddSelectedEntity( idEntity *ent );
	void					RemoveSelectedEntity( idEntity *ent );
	void					ClearSelectedEntities( void );
	void					DisplayEntities( void );
	bool					EntityIsSelectable( idEntity *ent, idVec4 *color = NULL, idStr *text = NULL );
private:
	int						nextSelectTime;
	idList<selectedTypeInfo_t> selectableEntityClasses;
	idList<idEntity *>		selectedEntities;
};

/*
===============================================================================

	Local game interface with methods for in-game editing.

===============================================================================
*/

class idGameEditLocal : public idGameEdit {
public:
	virtual						~idGameEditLocal( void ) {}

	// These are the canonical idDict to parameter parsing routines used by both the game and tools.
	virtual void				ParseSpawnArgsToRenderLight( const idDict *args, renderLight_t *renderLight );
	virtual void				ParseSpawnArgsToRenderEntity( const idDict *args, renderEntity_t *renderEntity );
	virtual void				ParseSpawnArgsToRefSound( const idDict *args, refSound_t *refSound );

	// Animation system calls for non-game based skeletal rendering.
	virtual idRenderModel *		ANIM_GetModelFromEntityDef( const char *classname );
	virtual const idVec3		&ANIM_GetModelOffsetFromEntityDef( const char *classname );
	virtual idRenderModel *		ANIM_GetModelFromEntityDef( const idDict *args );
	virtual idRenderModel *		ANIM_GetModelFromName( const char *modelName );
	virtual const idMD5Anim *	ANIM_GetAnimFromEntityDef( const char *classname, const char *animname );
	virtual int					ANIM_GetNumAnimsFromEntityDef( const idDict *args );
	virtual const char *		ANIM_GetAnimNameFromEntityDef( const idDict *args, int animNum );
	virtual const idMD5Anim *	ANIM_GetAnim( const char *fileName );
	virtual int					ANIM_GetLength( const idMD5Anim *anim );
	virtual int					ANIM_GetNumFrames( const idMD5Anim *anim );
	virtual void				ANIM_CreateAnimFrame( const idRenderModel *model, const idMD5Anim *anim, int numJoints, idJointMat *frame, int time, const idVec3 &offset, bool remove_origin_offset );
	virtual idRenderModel *		ANIM_CreateMeshForAnim( idRenderModel *model, const char *classname, const char *animname, int frame, bool remove_origin_offset );

	// HUMANHEAD pdm
	virtual const idMD5Anim *	ANIM_GetAnimFromArgs( const idDict *args, const char *animname );
	// HUMANHEAD END

	// Articulated Figure calls for AF editor and Radiant.
	virtual bool				AF_SpawnEntity( const char *fileName );
	virtual void				AF_UpdateEntities( const char *fileName );
	virtual void				AF_UndoChanges( void );
	virtual idRenderModel *		AF_CreateMesh( const idDict &args, idVec3 &meshOrigin, idMat3 &meshAxis, bool &poseIsSet );


	// Entity selection.
	virtual void				ClearEntitySelection( void );
	virtual int					GetSelectedEntities( idEntity *list[], int max );
	virtual void				AddSelectedEntity( idEntity *ent );

	// Selection methods
	virtual void				TriggerSelected();

	// Entity defs and spawning.
	virtual const idDict *		FindEntityDefDict( const char *name, bool makeDefault = true ) const;
	virtual void				SpawnEntityDef( const idDict &args, idEntity **ent );
	virtual idEntity *			FindEntity( const char *name ) const;
	virtual const char *		GetUniqueEntityName( const char *classname ) const;

	// Entity methods.
	virtual void				EntityGetOrigin( idEntity *ent, idVec3 &org ) const;
	virtual void				EntityGetAxis( idEntity *ent, idMat3 &axis ) const;
	virtual void				EntitySetOrigin( idEntity *ent, const idVec3 &org );
	virtual void				EntitySetAxis( idEntity *ent, const idMat3 &axis );
	virtual void				EntityTranslate( idEntity *ent, const idVec3 &org );
	virtual const idDict *		EntityGetSpawnArgs( idEntity *ent ) const;
	virtual void				EntityUpdateChangeableSpawnArgs( idEntity *ent, const idDict *dict );
	virtual void				EntityChangeSpawnArgs( idEntity *ent, const idDict *newArgs );
	virtual void				EntityUpdateVisuals( idEntity *ent );
	virtual void				EntitySetModel( idEntity *ent, const char *val );
	virtual void				EntityStopSound( idEntity *ent );
	virtual void				EntityDelete( idEntity *ent );
	virtual void				EntitySetColor( idEntity *ent, const idVec3 color );

	// Player methods.
	virtual bool				PlayerIsValid() const;
	virtual void				PlayerGetOrigin( idVec3 &org ) const;
	virtual void				PlayerGetAxis( idMat3 &axis ) const;
	virtual void				PlayerGetViewAngles( idAngles &angles ) const;
	virtual void				PlayerGetEyePosition( idVec3 &org ) const;

	// In game map editing support.
	virtual const idDict *		MapGetEntityDict( const char *name ) const;
	virtual void				MapSave( const char *path = NULL ) const;
	virtual void				MapSetEntityKeyVal( const char *name, const char *key, const char *val ) const ;
	virtual void				MapCopyDictToEntity( const char *name, const idDict *dict ) const;
	virtual int					MapGetUniqueMatchingKeyVals( const char *key, const char *list[], const int max ) const;
	virtual void				MapAddEntity( const idDict *dict ) const;
	virtual int					MapGetEntitiesMatchingClassWithString( const char *classname, const char *match, const char *list[], const int max ) const;
	virtual void				MapRemoveEntity( const char *name ) const;
	virtual void				MapEntityTranslate( const char *name, const idVec3 &v ) const;

	// In game script Debugging Support
	// IdProgram
	virtual void				GetLoadedScripts( idStrList **result );
	virtual bool				IsLineCode( const char *filename, int linenumber ) const;
	virtual const char *		GetFilenameForStatement( idProgram *program, int index ) const;
	virtual int					GetLineNumberForStatement( idProgram *program, int index ) const;

	// idInterpreter
	virtual bool				CheckForBreakPointHit( const idInterpreter *interpreter, const function_t *function1, const function_t *function2, int depth ) const;
	virtual bool				ReturnedFromFunction( const idProgram *program, const idInterpreter *interpreter, int index ) const;
	virtual bool				GetRegisterValue( const idInterpreter *interpreter, const char *name, idStr& out, int scopeDepth ) const;
	virtual const idThread*		GetThread( const idInterpreter *interpreter ) const;
	virtual int					GetInterpreterCallStackDepth( const idInterpreter *interpreter );
	virtual const function_t*	GetInterpreterCallStackFunction( const idInterpreter *interpreter, int stackDepth = -1 );

	// IdThread
	virtual const char *		ThreadGetName( const idThread *thread ) const;
	virtual int					ThreadGetNum( const idThread *thread ) const;
	virtual bool				ThreadIsDoneProcessing( const idThread *thread ) const;
	virtual bool				ThreadIsWaiting( const idThread *thread ) const;
	virtual bool				ThreadIsDying( const idThread *thread ) const;
	virtual int					GetTotalScriptThreads( ) const;
	virtual const idThread*		GetThreadByIndex( int index ) const;

	// MSG helpers
	virtual void				MSG_WriteThreadInfo( idBitMsg *msg, const idThread *thread, const idInterpreter *interpreter );
	virtual void				MSG_WriteCallstackFunc( idBitMsg *msg, const prstack_t *stack, const idProgram *program, int instructionPtr );
	virtual void				MSG_WriteInterpreterInfo( idBitMsg *msg, const idInterpreter *interpreter, const idProgram *program, int instructionPtr );
	virtual void				MSG_WriteScriptList( idBitMsg *msg );
};

#endif /* !__GAME_EDIT_H__ */
