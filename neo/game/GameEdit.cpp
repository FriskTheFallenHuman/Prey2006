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

#include "Game_local.h"


/*
===============================================================================

	Ingame cursor.

===============================================================================
*/

CLASS_DECLARATION( idEntity, idCursor3D )
END_CLASS

/*
===============
idCursor3D::idCursor3D
===============
*/
idCursor3D::idCursor3D( void ) {
	draggedPosition.Zero();
}

/*
===============
idCursor3D::~idCursor3D
===============
*/
idCursor3D::~idCursor3D( void ) {
}

/*
===============
idCursor3D::Spawn
===============
*/
void idCursor3D::Spawn( void ) {
}

/*
===============
idCursor3D::Present
===============
*/
void idCursor3D::Present( void ) {
	// don't present to the renderer if the entity hasn't changed
	if ( !( thinkFlags & TH_UPDATEVISUALS ) ) {
		return;
	}
	BecomeInactive( TH_UPDATEVISUALS );

	const idVec3 &origin = GetPhysics()->GetOrigin();
	const idMat3 &axis = GetPhysics()->GetAxis();
	gameRenderWorld->DebugArrow( colorYellow, origin + axis[1] * -5.0f + axis[2] * 5.0f, origin, 2 );
	gameRenderWorld->DebugArrow( colorRed, origin, draggedPosition, 2 );
}

/*
===============
idCursor3D::Think
===============
*/
void idCursor3D::Think( void ) {
	if ( thinkFlags & TH_THINK ) {
		drag.Evaluate( gameLocal.time );
	}
	Present();
}


/*
===============================================================================

	Allows entities to be dragged through the world with physics.

===============================================================================
*/

#define MAX_DRAG_TRACE_DISTANCE			2048.0f

/*
==============
idDragEntity::idDragEntity
==============
*/
idDragEntity::idDragEntity( void ) {
	cursor = NULL;
	Clear();
}

/*
==============
idDragEntity::~idDragEntity
==============
*/
idDragEntity::~idDragEntity( void ) {
	StopDrag();
	selected = NULL;
	delete cursor;
	cursor = NULL;
}


/*
==============
idDragEntity::Clear
==============
*/
void idDragEntity::Clear() {
	dragEnt			= NULL;
	joint			= INVALID_JOINT;
	id				= 0;
	localEntityPoint.Zero();
	localPlayerPoint.Zero();
	bodyName.Clear();
	selected		= NULL;
}

/*
==============
idDragEntity::StopDrag
==============
*/
void idDragEntity::StopDrag( void ) {
	dragEnt = NULL;
	if ( cursor ) {
		cursor->BecomeInactive( TH_THINK );
	}
}

/*
==============
idDragEntity::Update
==============
*/
void idDragEntity::Update( idPlayer *player ) {
	idVec3 viewPoint, origin;
	idMat3 viewAxis, axis;
	trace_t trace;
	idEntity *newEnt;
	idAngles angles;
	jointHandle_t newJoint = INVALID_JOINT;
	idStr newBodyName;

	player->GetViewPos( viewPoint, viewAxis );

	// if no entity selected for dragging
	if ( !dragEnt.GetEntity() ) {

		if ( player->usercmd.buttons & BUTTON_ATTACK ) {

			gameLocal.clip.TracePoint( trace, viewPoint, viewPoint + viewAxis[0] * MAX_DRAG_TRACE_DISTANCE, (CONTENTS_SOLID|CONTENTS_RENDERMODEL|CONTENTS_BODY), player );
			if ( trace.fraction < 1.0f ) {

				newEnt = gameLocal.entities[ trace.c.entityNum ];
				if ( newEnt ) {

					if ( newEnt->GetBindMaster() ) {
						if ( newEnt->GetBindJoint() ) {
							trace.c.id = JOINT_HANDLE_TO_CLIPMODEL_ID( newEnt->GetBindJoint() );
						} else {
							trace.c.id = newEnt->GetBindBody();
						}
						newEnt = newEnt->GetBindMaster();
					}

					if ( newEnt->IsType( idAFEntity_Base::Type ) && static_cast<idAFEntity_Base *>(newEnt)->IsActiveAF() ) {
						idAFEntity_Base *af = static_cast<idAFEntity_Base *>(newEnt);

						// joint being dragged
						newJoint = CLIPMODEL_ID_TO_JOINT_HANDLE( trace.c.id );
						// get the body id from the trace model id which might be a joint handle
						trace.c.id = af->BodyForClipModelId( trace.c.id );
						// get the name of the body being dragged
						newBodyName = af->GetAFPhysics()->GetBody( trace.c.id )->GetName();

					} else if ( !newEnt->IsType( idWorldspawn::Type ) ) {

						if ( trace.c.id < 0 ) {
							newJoint = CLIPMODEL_ID_TO_JOINT_HANDLE( trace.c.id );
						} else {
							newJoint = INVALID_JOINT;
						}
						newBodyName = "";

					} else {

						newJoint = INVALID_JOINT;
						newEnt = NULL;
					}
				}
				if ( newEnt ) {
					dragEnt = newEnt;
					selected = newEnt;
					joint = newJoint;
					id = trace.c.id;
					bodyName = newBodyName;

					if ( !cursor ) {
						cursor = ( idCursor3D * )gameLocal.SpawnEntityType( idCursor3D::Type );
					}

					idPhysics *phys = dragEnt.GetEntity()->GetPhysics();
					localPlayerPoint = ( trace.c.point - viewPoint ) * viewAxis.Transpose();
					origin = phys->GetOrigin( id );
					axis = phys->GetAxis( id );
					localEntityPoint = ( trace.c.point - origin ) * axis.Transpose();

					cursor->drag.Init( g_dragDamping.GetFloat() );
					cursor->drag.SetPhysics( phys, id, localEntityPoint );
					cursor->Show();

					if ( phys->IsType( idPhysics_AF::Type ) ||
							phys->IsType( idPhysics_RigidBody::Type ) ||
								phys->IsType( idPhysics_Monster::Type ) ) {
						cursor->BecomeActive( TH_THINK );
					}
				}
			}
		}
	}

	// if there is an entity selected for dragging
	idEntity *drag = dragEnt.GetEntity();
	if ( drag ) {

		if ( !( player->usercmd.buttons & BUTTON_ATTACK ) ) {
			StopDrag();
			return;
		}

		cursor->SetOrigin( viewPoint + localPlayerPoint * viewAxis );
		cursor->SetAxis( viewAxis );

		cursor->drag.SetDragPosition( cursor->GetPhysics()->GetOrigin() );

		renderEntity_t *renderEntity = drag->GetRenderEntity();
		idAnimator *dragAnimator = drag->GetAnimator();

		if ( joint != INVALID_JOINT && renderEntity && dragAnimator ) {
			dragAnimator->GetJointTransform( joint, gameLocal.time, cursor->draggedPosition, axis );
			cursor->draggedPosition = renderEntity->origin + cursor->draggedPosition * renderEntity->axis;
			gameRenderWorld->DrawText( va( "%s\n%s\n%s, %s", drag->GetName(), drag->GetType()->classname, dragAnimator->GetJointName( joint ), bodyName.c_str() ), cursor->GetPhysics()->GetOrigin(), 0.1f, colorWhite, viewAxis, 1 );
		} else {
			cursor->draggedPosition = cursor->GetPhysics()->GetOrigin();
			gameRenderWorld->DrawText( va( "%s\n%s\n%s", drag->GetName(), drag->GetType()->classname, bodyName.c_str() ), cursor->GetPhysics()->GetOrigin(), 0.1f, colorWhite, viewAxis, 1 );
		}
	}

	// if there is a selected entity
	if ( selected.GetEntity() && g_dragShowSelection.GetBool() ) {
		// draw the bbox of the selected entity
		renderEntity_t *renderEntity = selected.GetEntity()->GetRenderEntity();
		if ( renderEntity ) {
			gameRenderWorld->DebugBox( colorYellow, idBox( renderEntity->bounds, renderEntity->origin, renderEntity->axis ) );
		}
	}
}

/*
==============
idDragEntity::SetSelected
==============
*/
void idDragEntity::SetSelected( idEntity *ent ) {
	selected = ent;
	StopDrag();
}

/*
==============
idDragEntity::DeleteSelected
==============
*/
void idDragEntity::DeleteSelected( void ) {
	delete selected.GetEntity();
	selected = NULL;
	StopDrag();
}

/*
==============
idDragEntity::BindSelected
==============
*/
void idDragEntity::BindSelected( void ) {
	int num, largestNum;
	idLexer lexer;
	idToken type, bodyName;
	idStr key, value, bindBodyName;
	const idKeyValue *kv;
	idAFEntity_Base *af;

	af = static_cast<idAFEntity_Base *>(dragEnt.GetEntity());

	if ( !af || !af->IsType( idAFEntity_Base::Type ) || !af->IsActiveAF() ) {
		return;
	}

	bindBodyName = af->GetAFPhysics()->GetBody( id )->GetName();
	largestNum = 1;

	// parse all the bind constraints
	kv = af->spawnArgs.MatchPrefix( "bindConstraint ", NULL );
	while ( kv ) {
		key = kv->GetKey();
		key.Strip( "bindConstraint " );
		if ( sscanf( key, "bind%d", &num ) ) {
			if ( num >= largestNum ) {
				largestNum = num + 1;
			}
		}

		lexer.LoadMemory( kv->GetValue(), kv->GetValue().Length(), kv->GetKey() );
		lexer.ReadToken( &type );
		lexer.ReadToken( &bodyName );
		lexer.FreeSource();

		// if there already exists a bind constraint for this body
		if ( bodyName.Icmp( bindBodyName ) == 0 ) {
			// delete the bind constraint
			af->spawnArgs.Delete( kv->GetKey() );
			kv = NULL;
		}

		kv = af->spawnArgs.MatchPrefix( "bindConstraint ", kv );
	}

	sprintf( key, "bindConstraint bind%d", largestNum );
	sprintf( value, "ballAndSocket %s %s", bindBodyName.c_str(), af->GetAnimator()->GetJointName( joint ) );

	af->spawnArgs.Set( key, value );
	af->spawnArgs.Set( "bind", "worldspawn" );
	af->Bind( gameLocal.world, true );
}

/*
==============
idDragEntity::UnbindSelected
==============
*/
void idDragEntity::UnbindSelected( void ) {
	const idKeyValue *kv;
	idAFEntity_Base *af;

	af = static_cast<idAFEntity_Base *>(selected.GetEntity());

	if ( !af || !af->IsType( idAFEntity_Base::Type ) || !af->IsActiveAF() ) {
		return;
	}

	// unbind the selected entity
	af->Unbind();

	// delete all the bind constraints
	kv = selected.GetEntity()->spawnArgs.MatchPrefix( "bindConstraint ", NULL );
	while ( kv ) {
		selected.GetEntity()->spawnArgs.Delete( kv->GetKey() );
		kv = selected.GetEntity()->spawnArgs.MatchPrefix( "bindConstraint ", NULL );
	}

	// delete any bind information
	af->spawnArgs.Delete( "bind" );
	af->spawnArgs.Delete( "bindToJoint" );
	af->spawnArgs.Delete( "bindToBody" );
}


/*
===============================================================================

	Handles ingame entity editing.

===============================================================================
*/

/*
==============
idEditEntities::idEditEntities
==============
*/
idEditEntities::idEditEntities( void ) {
	selectableEntityClasses.Clear();
	nextSelectTime = 0;
}

/*
=============
idEditEntities::SelectEntity
=============
*/
bool idEditEntities::SelectEntity( const idVec3 &origin, const idVec3 &dir, const idEntity *skip ) {
	idVec3		end;
	idEntity	*ent;

	if ( !g_editEntityMode.GetInteger() || selectableEntityClasses.Num() == 0 ) {
		return false;
	}

	if ( gameLocal.time < nextSelectTime ) {
		return true;
	}
	nextSelectTime = gameLocal.time + 300;

	end = origin + dir * 4096.0f;

	ent = NULL;
	for ( int i = 0; i < selectableEntityClasses.Num(); i++ ) {
		ent = gameLocal.FindTraceEntity( origin, end, *selectableEntityClasses[i].typeInfo, skip );
		if ( ent ) {
			break;
		}
	}
	if ( ent ) {
		ClearSelectedEntities();
		if ( EntityIsSelectable( ent ) ) {
			AddSelectedEntity( ent );
			gameLocal.Printf( "entity #%d: %s '%s'\n", ent->entityNumber, ent->GetClassname(), ent->name.c_str() );
			ent->ShowEditingDialog();
			return true;
		}
	}
	return false;
}

/*
=============
idEditEntities::AddSelectedEntity
=============
*/
void idEditEntities::AddSelectedEntity(idEntity *ent) {
	ent->fl.selected = true;
	selectedEntities.AddUnique(ent);
}

/*
==============
idEditEntities::RemoveSelectedEntity
==============
*/
void idEditEntities::RemoveSelectedEntity( idEntity *ent ) {
	if ( selectedEntities.Find( ent ) ) {
		selectedEntities.Remove( ent );
	}
}

/*
=============
idEditEntities::ClearSelectedEntities
=============
*/
void idEditEntities::ClearSelectedEntities() {
	int i, count;

	count = selectedEntities.Num();
	for ( i = 0; i < count; i++ ) {
		selectedEntities[i]->fl.selected = false;
	}
	selectedEntities.Clear();
}


/*
=============
idEditEntities::EntityIsSelectable
=============
*/
bool idEditEntities::EntityIsSelectable( idEntity *ent, idVec4 *color, idStr *text ) {
	for ( int i = 0; i < selectableEntityClasses.Num(); i++ ) {
		//HUMANHEAD: aob - changed to inheritance check.  Hopefully no breakage.  :)
		if ( ent->IsType(*selectableEntityClasses[i].typeInfo) ) {
			if ( text ) {
				*text = selectableEntityClasses[i].textKey;
			}
			if ( color ) {
				if ( ent->fl.selected ) {
					*color = colorRed;
				} else {
					switch( i ) {
					case 1 :
						*color = colorYellow;
						break;
					case 2 :
						*color = colorBlue;
						break;
					default:
						*color = colorGreen;
					}
				}
			}
			return true;
		}
	}
	return false;
}

/*
=============
idEditEntities::DisplayEntities
=============
*/
void idEditEntities::DisplayEntities( void ) {
	idEntity *ent;

	if ( !gameLocal.GetLocalPlayer() ) {
		return;
	}

	selectableEntityClasses.Clear();
	selectedTypeInfo_t sit;

	switch( g_editEntityMode.GetInteger() ) {
		case 1:
			sit.typeInfo = &idLight::Type;
			sit.textKey = "texture";
			selectableEntityClasses.Append( sit );
			break;
		case 2:
			sit.typeInfo = &idSound::Type;
			sit.textKey = "s_shader|name";
			selectableEntityClasses.Append( sit );
			break;
		case 3:
			sit.typeInfo = &idAFEntity_Base::Type;
			sit.textKey = "articulatedFigure";
			selectableEntityClasses.Append( sit );
			break;
		case 4:
			sit.typeInfo = &idFuncEmitter::Type;
			sit.textKey = "model";
			selectableEntityClasses.Append( sit );
			break;
		case 5:
			sit.typeInfo = &idAI::Type;
			sit.textKey = "name";
			selectableEntityClasses.Append( sit );
			break;
		case 6:
			sit.typeInfo = &idEntity::Type;
			sit.textKey = "name";
			selectableEntityClasses.Append( sit );
			break;
		case 7:
			sit.typeInfo = &idEntity::Type;
			sit.textKey = "model";
			selectableEntityClasses.Append( sit );
			break;
		default:
			return;
	}

	idBounds viewBounds( gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin() );
	idBounds viewTextBounds( gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin() );
	idMat3 axis = gameLocal.GetLocalPlayer()->viewAngles.ToMat3();

	viewBounds.ExpandSelf( g_editEntityDistance.GetFloat() );
	viewTextBounds.ExpandSelf( g_editEntityTextDistance.GetFloat() );

	idStr textKey;
	idStr textKey2;
	idStr strOutput;

	for( ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {

		idVec4 color;

		textKey = "";
		if ( !EntityIsSelectable( ent, &color, &textKey ) ) {
			continue;
		}

		textKey2 = "";
		int iIndex = textKey.Find( '|' );
		if ( iIndex >= 0 ) {
			textKey2 = textKey.Mid ( iIndex + 1, textKey.Length() - ( iIndex + 1 ) );	// hmmm, they emulate 99% of MS CString but don't have a single-param Mid() func?
			textKey  = textKey.Left( iIndex );
		}

		bool drawArrows = false;
		bool drawDirection = false;

		if ( ent->GetType() == &idAFEntity_Base::Type ) {
			if ( !static_cast<idAFEntity_Base *>(ent)->IsActiveAF() ) {
				continue;
			}
		} else if ( ent->IsType( hhSound::Type ) ) {	// HUMANHEAD pdm: Changed to proper hhSound check
			if ( ent->fl.selected ) {
				drawArrows = true;
			}
			ent->UpdateSound();
			const idSoundShader * ss = declManager->FindSound( ent->spawnArgs.GetString( textKey ) );
			if ( ss->HasDefaultSound() || ss->base->GetState() == DS_DEFAULTED ) {
				color.Set( 1.0f, 0.0f, 1.0f, 1.0f );
			}
		} else if ( ent->GetType() == &idFuncEmitter::Type ) {
			if ( ent->fl.selected ) {
				drawArrows = true;
				drawDirection = true;
			}
		} else if ( ent->GetType() == &idLight::Type ) {
			if ( ent->fl.selected ) {
				drawArrows = true;
				drawDirection = true;

				idLight* light = static_cast<idLight*>( ent );

				cvarSystem->SetCVarInteger( "r_singleLight", light->GetLightDefHandle() );
				cvarSystem->SetCVarInteger( "r_showLights", 3 );

				renderLight_t renderLight = light->GetRenderLight();

				// draw arrow from entity origin to globalLightOrigin

				idVec3 globalLightOrigin;
				if ( renderLight.parallel ) {
					idVec3 dir = renderLight.lightCenter;
					if ( dir.Normalize() == 0.0f ) {
						// make point straight up if not specified
						dir[2] = 1.0f;
					}
					globalLightOrigin = renderLight.origin + dir * 100000.0f;
				} else {
					globalLightOrigin = renderLight.origin + renderLight.axis * renderLight.lightCenter;
				}

				idVec3 start = ent->GetPhysics()->GetOrigin();
				idVec3 end = globalLightOrigin;

				gameRenderWorld->DebugArrow( colorYellow, start, end, 2 );

				if ( !renderLight.parallel ) {
					gameRenderWorld->DrawText( "globalLightOrigin", end + idVec3( 4, 0, 0 ), 0.15f, colorYellow, axis );
				}
			}
		}

		if ( !viewBounds.ContainsPoint( ent->GetPhysics()->GetOrigin() ) ) {
			continue;
		}

		gameRenderWorld->DebugBounds( color, idBounds( ent->GetPhysics()->GetOrigin() ).Expand( 8 ) );
		if ( drawArrows ) {
			idVec3 start = ent->GetPhysics()->GetOrigin();
			idVec3 end = start + idVec3( 1, 0, 0 ) * 20.0f;
			gameRenderWorld->DebugArrow( colorWhite, start, end, 2 );
			gameRenderWorld->DrawText( "x+", end + idVec3( 4, 0, 0 ), 0.15f, colorWhite, axis );
			end = start + idVec3( 1, 0, 0 ) * -20.0f;
			gameRenderWorld->DebugArrow( colorWhite, start, end, 2 );
			gameRenderWorld->DrawText( "x-", end + idVec3( -4, 0, 0 ), 0.15f, colorWhite, axis );
			end = start + idVec3( 0, 1, 0 ) * +20.0f;
			gameRenderWorld->DebugArrow( colorGreen, start, end, 2 );
			gameRenderWorld->DrawText( "y+", end + idVec3( 0, 4, 0 ), 0.15f, colorWhite, axis );
			end = start + idVec3( 0, 1, 0 ) * -20.0f;
			gameRenderWorld->DebugArrow( colorGreen, start, end, 2 );
			gameRenderWorld->DrawText( "y-", end + idVec3( 0, -4, 0 ), 0.15f, colorWhite, axis );
			end = start + idVec3( 0, 0, 1 ) * +20.0f;
			gameRenderWorld->DebugArrow( colorBlue, start, end, 2 );
			gameRenderWorld->DrawText( "z+", end + idVec3( 0, 0, 4 ), 0.15f, colorWhite, axis );
			end = start + idVec3( 0, 0, 1 ) * -20.0f;
			gameRenderWorld->DebugArrow( colorBlue, start, end, 2 );
			gameRenderWorld->DrawText( "z-", end + idVec3( 0, 0, -4 ), 0.15f, colorWhite, axis );
		}

		if ( drawDirection ) {
			idVec3 start = ent->GetPhysics()->GetOrigin ( );
			idVec3 end   = start + ent->GetPhysics()->GetAxis()[0] * 35.0f;
			gameRenderWorld->DebugArrow ( colorYellow, start, end, 6 );
		}

		if ( textKey.Length() ) {
			if ( viewTextBounds.ContainsPoint( ent->GetPhysics()->GetOrigin() ) ) {
				strOutput = ent->spawnArgs.GetString( textKey );
				if ( !textKey2.IsEmpty() ) {
					strOutput += " ( ";
					strOutput += ent->spawnArgs.GetString( textKey2 );
					strOutput += " )";
				}
				gameRenderWorld->DrawText( strOutput.c_str(), ent->GetPhysics()->GetOrigin() + idVec3( 0, 0, 12 ), 0.25, colorWhite, axis, 1 );
			}
		}
	}
}


/*
===============================================================================

	Local game interface with methods for in-game editing.

===============================================================================
*/

// the rest of the engine will only reference the "gameEdit" variable, while all local aspects stay hidden
idGameEditLocal		gameEditLocal;
idGameEdit *		gameEdit = &gameEditLocal;	// statically pointed at an idGameEditLocal


/*
=============
idGameEditLocal::GetSelectedEntities
=============
*/
int idGameEditLocal::GetSelectedEntities( idEntity *list[], int max ) {
	int num = 0;
	idEntity *ent;

	for( ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
		if ( ent->fl.selected ) {
			list[num++] = ent;
			if ( num >= max ) {
				break;
			}
		}
	}
	return num;
}

/*
=============
idGameEditLocal::TriggerSelected
=============
*/
void idGameEditLocal::TriggerSelected() {
	idEntity *ent;
	for( ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
		if ( ent->fl.selected ) {
			ent->ProcessEvent( &EV_Activate, gameLocal.GetLocalPlayer() );
		}
	}
}

/*
================
idGameEditLocal::ClearEntitySelection
================
*/
void idGameEditLocal::ClearEntitySelection() {
	idEntity *ent;

	for( ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
		ent->fl.selected = false;
	}

	if ( gameLocal.editEntities ) {
		gameLocal.editEntities->ClearSelectedEntities();
	}
}

/*
================
idGameEditLocal::AddSelectedEntity
================
*/
void idGameEditLocal::AddSelectedEntity( idEntity *ent ) {
	if ( ent && gameLocal.editEntities ) {
		gameLocal.editEntities->AddSelectedEntity( ent );
	}
}

/*
================
idGameEditLocal::FindEntityDefDict
================
*/
const idDict *idGameEditLocal::FindEntityDefDict( const char *name, bool makeDefault ) const {
	return gameLocal.FindEntityDefDict( name, makeDefault );
}

/*
================
idGameEditLocal::SpawnEntityDef
================
*/
void idGameEditLocal::SpawnEntityDef( const idDict &args, idEntity **ent ) {
	gameLocal.SpawnEntityDef( args, ent );
}

/*
================
idGameEditLocal::FindEntity
================
*/
idEntity *idGameEditLocal::FindEntity( const char *name ) const {
	return gameLocal.FindEntity( name );
}

/*
=============
idGameEditLocal::GetUniqueEntityName

generates a unique name for a given classname
=============
*/
const char *idGameEditLocal::GetUniqueEntityName( const char *classname ) const {
	int			id;
	static char	name[1024];

	// can only have MAX_GENTITIES, so if we have a spot available, we're guaranteed to find one
	for( id = 0; id < MAX_GENTITIES; id++ ) {
		idStr::snPrintf( name, sizeof( name ), "%s_%d", classname, id );
		if ( !gameLocal.FindEntity( name ) ) {
			return name;
		}
	}

	// id == MAX_GENTITIES + 1, which can't be in use if we get here
	idStr::snPrintf( name, sizeof( name ), "%s_%d", classname, id );
	return name;
}

/*
================
idGameEditLocal::EntityGetOrigin
================
*/
void  idGameEditLocal::EntityGetOrigin( idEntity *ent, idVec3 &org ) const {
	if ( ent ) {
		org = ent->GetPhysics()->GetOrigin();
	}
}

/*
================
idGameEditLocal::EntityGetAxis
================
*/
void idGameEditLocal::EntityGetAxis( idEntity *ent, idMat3 &axis ) const {
	if ( ent ) {
		axis = ent->GetPhysics()->GetAxis();
	}
}

/*
================
idGameEditLocal::EntitySetOrigin
================
*/
void idGameEditLocal::EntitySetOrigin( idEntity *ent, const idVec3 &org ) {
	if ( ent ) {
		ent->SetOrigin( org );
	}
}

/*
================
idGameEditLocal::EntitySetAxis
================
*/
void idGameEditLocal::EntitySetAxis( idEntity *ent, const idMat3 &axis ) {
	if ( ent ) {
		ent->SetAxis( axis );
	}
}

/*
================
idGameEditLocal::EntitySetColor
================
*/
void idGameEditLocal::EntitySetColor( idEntity *ent, const idVec3 color ) {
	if ( ent ) {
		ent->SetColor( color );
	}
}

/*
================
idGameEditLocal::EntityTranslate
================
*/
void idGameEditLocal::EntityTranslate( idEntity *ent, const idVec3 &org ) {
	if ( ent ) {
		ent->GetPhysics()->Translate( org );
	}
}

/*
================
idGameEditLocal::EntityGetSpawnArgs
================
*/
const idDict *idGameEditLocal::EntityGetSpawnArgs( idEntity *ent ) const {
	if ( ent ) {
		return &ent->spawnArgs;
	}
	return NULL;
}

/*
================
idGameEditLocal::EntityUpdateChangeableSpawnArgs
================
*/
void idGameEditLocal::EntityUpdateChangeableSpawnArgs( idEntity *ent, const idDict *dict ) {
	if ( ent ) {
		ent->UpdateChangeableSpawnArgs( dict );
	}
}

/*
================
idGameEditLocal::EntityChangeSpawnArgs
================
*/
void idGameEditLocal::EntityChangeSpawnArgs( idEntity *ent, const idDict *newArgs ) {
	if ( ent ) {
		for ( int i = 0 ; i < newArgs->GetNumKeyVals () ; i ++ ) {
			const idKeyValue *kv = newArgs->GetKeyVal( i );

			if ( kv->GetValue().Length() > 0 ) {
				ent->spawnArgs.Set ( kv->GetKey() ,kv->GetValue() );
			} else {
				ent->spawnArgs.Delete ( kv->GetKey() );
			}
		}
	}
}

/*
================
idGameEditLocal::EntityUpdateVisuals
================
*/
void idGameEditLocal::EntityUpdateVisuals( idEntity *ent ) {
	if ( ent ) {
		ent->UpdateVisuals();
	}
}

/*
================
idGameEditLocal::EntitySetModel
================
*/
void idGameEditLocal::EntitySetModel( idEntity *ent, const char *val ) {
	if ( ent ) {
		ent->spawnArgs.Set( "model", val );
		ent->SetModel( val );
	}
}

/*
================
idGameEditLocal::EntityStopSound
================
*/
void idGameEditLocal::EntityStopSound( idEntity *ent ) {
	if ( ent ) {
		ent->StopSound( SND_CHANNEL_ANY, false );
	}
}

/*
================
idGameEditLocal::EntityDelete
================
*/
void idGameEditLocal::EntityDelete( idEntity *ent ) {
	delete ent;
}

/*
================
idGameEditLocal::PlayerIsValid
================
*/
bool idGameEditLocal::PlayerIsValid() const {
	return ( gameLocal.GetLocalPlayer() != NULL );
}

/*
================
idGameEditLocal::PlayerGetOrigin
================
*/
void idGameEditLocal::PlayerGetOrigin( idVec3 &org ) const {
	org = gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin();
}

/*
================
idGameEditLocal::PlayerGetAxis
================
*/
void idGameEditLocal::PlayerGetAxis( idMat3 &axis ) const {
	axis = gameLocal.GetLocalPlayer()->GetPhysics()->GetAxis();
}

/*
================
idGameEditLocal::PlayerGetViewAngles
================
*/
void idGameEditLocal::PlayerGetViewAngles( idAngles &angles ) const {
	angles = gameLocal.GetLocalPlayer()->viewAngles;
}

/*
================
idGameEditLocal::PlayerGetEyePosition
================
*/
void idGameEditLocal::PlayerGetEyePosition( idVec3 &org ) const {
	org = gameLocal.GetLocalPlayer()->GetEyePosition();
}


/*
================
idGameEditLocal::MapGetEntityDict
================
*/
const idDict *idGameEditLocal::MapGetEntityDict( const char *name ) const {
	idMapFile *mapFile = gameLocal.GetLevelMap();
	if ( mapFile && name && *name ) {
		idMapEntity *mapent = mapFile->FindEntity( name );
		if ( mapent ) {
			return &mapent->epairs;
		}
	}
	return NULL;
}

/*
================
idGameEditLocal::MapSave
================
*/
void idGameEditLocal::MapSave( const char *path ) const {
	idMapFile *mapFile = gameLocal.GetLevelMap();
	if (mapFile) {
		mapFile->Write( (path) ? path : mapFile->GetName(), ".map");
	}
}

/*
================
idGameEditLocal::MapSetEntityKeyVal
================
*/
void idGameEditLocal::MapSetEntityKeyVal( const char *name, const char *key, const char *val ) const {
	idMapFile *mapFile = gameLocal.GetLevelMap();
	if ( mapFile && name && *name ) {
		idMapEntity *mapent = mapFile->FindEntity( name );
		if ( mapent ) {
			mapent->epairs.Set( key, val );
		}
	}
}

/*
================
idGameEditLocal::MapCopyDictToEntity
================
*/
void idGameEditLocal::MapCopyDictToEntity( const char *name, const idDict *dict ) const {
	idMapFile *mapFile = gameLocal.GetLevelMap();
	if ( mapFile && name && *name ) {
		idMapEntity *mapent = mapFile->FindEntity( name );
		if ( mapent ) {
			for ( int i = 0; i < dict->GetNumKeyVals(); i++ ) {
				const idKeyValue *kv = dict->GetKeyVal( i );
				const char *key = kv->GetKey();
				const char *val = kv->GetValue();
				mapent->epairs.Set( key, val );
			}
		}
	}
}



/*
================
idGameEditLocal::MapGetUniqueMatchingKeyVals
================
*/
int idGameEditLocal::MapGetUniqueMatchingKeyVals( const char *key, const char *list[], int max ) const {
	idMapFile *mapFile = gameLocal.GetLevelMap();
	int count = 0;
	if ( mapFile ) {
		for ( int i = 0; i < mapFile->GetNumEntities(); i++ ) {
			idMapEntity *ent = mapFile->GetEntity( i );
			if ( ent ) {
				const char *k = ent->epairs.GetString( key );
				if ( k && *k && count < max ) {
					list[count++] = k;
				}
			}
		}
	}
	return count;
}

/*
================
idGameEditLocal::MapAddEntity
================
*/
void idGameEditLocal::MapAddEntity( const idDict *dict ) const {
	idMapFile *mapFile = gameLocal.GetLevelMap();
	if ( mapFile ) {
		idMapEntity *ent = new idMapEntity();
		ent->epairs = *dict;
		mapFile->AddEntity( ent );
	}
}

/*
================
idGameEditLocal::MapRemoveEntity
================
*/
void idGameEditLocal::MapRemoveEntity( const char *name ) const {
	idMapFile *mapFile = gameLocal.GetLevelMap();
	if ( mapFile ) {
		idMapEntity *ent = mapFile->FindEntity( name );
		if ( ent ) {
			mapFile->RemoveEntity( ent );
		}
	}
}


/*
================
idGameEditLocal::MapGetEntitiesMatchignClassWithString
================
*/
int idGameEditLocal::MapGetEntitiesMatchingClassWithString( const char *classname, const char *match, const char *list[], const int max ) const {
	idMapFile *mapFile = gameLocal.GetLevelMap();
	int count = 0;
	if ( mapFile ) {
		int entCount = mapFile->GetNumEntities();
		for ( int i = 0 ; i < entCount; i++ ) {
			idMapEntity *ent = mapFile->GetEntity(i);
			if (ent) {
				idStr work = ent->epairs.GetString("classname");
				if ( work.Icmp( classname ) == 0 ) {
					if ( match && *match ) {
						work = ent->epairs.GetString( "soundgroup" );
						if ( count < max && work.Icmp( match ) == 0 ) {
							list[count++] = ent->epairs.GetString( "name" );
						}
					} else if ( count < max ) {
						list[count++] = ent->epairs.GetString( "name" );
					}
				}
			}
		}
	}
	return count;
}


/*
================
idGameEditLocal::MapEntityTranslate
================
*/
void idGameEditLocal::MapEntityTranslate( const char *name, const idVec3 &v ) const {
	idMapFile *mapFile = gameLocal.GetLevelMap();
	if ( mapFile && name && *name ) {
		idMapEntity *mapent = mapFile->FindEntity( name );
		if ( mapent ) {
			idVec3 origin;
			mapent->epairs.GetVector( "origin", "", origin );
			origin += v;
			mapent->epairs.SetVector( "origin", origin );
		}
	}
}

/*
===============================================================================

  editor support routines

===============================================================================
*/


/*
================
idGameEditLocal::AF_SpawnEntity
================
*/
bool idGameEditLocal::AF_SpawnEntity( const char *fileName ) {
	idDict args;
	idPlayer *player;
	idAFEntity_Generic *ent;
	const idDeclAF *af;
	idVec3 org;
	float yaw;

	player = gameLocal.GetLocalPlayer();
	if ( !player || !gameLocal.CheatsOk( false ) ) {
		return false;
	}

	af = static_cast<const idDeclAF *>( declManager->FindType( DECL_AF, fileName ) );
	if ( !af ) {
		return false;
	}

	yaw = player->viewAngles.yaw;
	args.Set( "angle", va( "%f", yaw + 180 ) );
	org = player->GetPhysics()->GetOrigin() + idAngles( 0, yaw, 0 ).ToForward() * 80 + idVec3( 0, 0, 1 );
	args.Set( "origin", org.ToString() );
	// HUMANHEAD nla - Need them to spawn a hhAFEntity
#ifdef HUMANHEAD
	args.Set( "spawnclass", "hhAFEntity" );
#else
	args.Set( "spawnclass", "idAFEntity_Generic" );
#endif	// HUMANHEAD END
	if ( af->model[0] ) {
		args.Set( "model", af->model.c_str() );
	} else {
		args.Set( "model", fileName );
	}
	if ( af->skin[0] ) {
		args.Set( "skin", af->skin.c_str() );
	}
	args.Set( "articulatedFigure", fileName );
	args.Set( "nodrop", "1" );
	// HUMANHEAD nla - Need them to spawn a hhAFEntity
#ifdef HUMANHEAD
	ent = static_cast<idAFEntity_Generic *>( gameLocal.SpawnObject( "ragdoll_base", &args ) );
#else
	ent = static_cast<idAFEntity_Generic *>(gameLocal.SpawnEntityType( idAFEntity_Generic::Type, &args));
#endif	// HUMANHEAD END

	// always update this entity
	ent->BecomeActive( TH_THINK );
	ent->KeepRunningPhysics();
	ent->fl.forcePhysicsUpdate = true;

	player->dragEntity.SetSelected( ent );

	return true;
}

/*
================
idGameEditLocal::AF_UpdateEntities
================
*/
void idGameEditLocal::AF_UpdateEntities( const char *fileName ) {
	idEntity *ent;
	idAFEntity_Base *af;
	idStr name;

	name = fileName;
	name.StripFileExtension();

	// reload any idAFEntity_Generic which uses the given articulated figure file
	for( ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
		if ( ent->IsType( idAFEntity_Base::Type ) ) {
			af = static_cast<idAFEntity_Base *>(ent);
			if ( name.Icmp( af->GetAFName() ) == 0 ) {
				af->LoadAF();
				af->GetAFPhysics()->PutToRest();
			}
		}
	}
}

/*
================
idGameEditLocal::AF_UndoChanges
================
*/
void idGameEditLocal::AF_UndoChanges( void ) {
	int i, c;
	idEntity *ent;
	idAFEntity_Base *af;
	idDeclAF *decl;

	c = declManager->GetNumDecls( DECL_AF );
	for ( i = 0; i < c; i++ ) {
		decl = static_cast<idDeclAF *>( const_cast<idDecl *>( declManager->DeclByIndex( DECL_AF, i, false ) ) );
		if ( !decl->modified ) {
			continue;
		}

		decl->Invalidate();
		declManager->FindType( DECL_AF, decl->GetName() );

		// reload all AF entities using the file
		for( ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
			if ( ent->IsType( idAFEntity_Base::Type ) ) {
				af = static_cast<idAFEntity_Base *>(ent);
				if ( idStr::Icmp( decl->GetName(), af->GetAFName() ) == 0 ) {
					af->LoadAF();
				}
			}
		}
	}
}

/*
================
GetJointTransform
================
*/
typedef struct {
	renderEntity_t *ent;
	const idMD5Joint *joints;
} jointTransformData_t;

static bool GetJointTransform( void *model, const idJointMat *frame, const char *jointName, idVec3 &origin, idMat3 &axis ) {
	int i;
	jointTransformData_t *data = reinterpret_cast<jointTransformData_t *>(model);

	for ( i = 0; i < data->ent->numJoints; i++ ) {
		if ( data->joints[i].name.Icmp( jointName ) == 0 ) {
			break;
		}
	}
	if ( i >= data->ent->numJoints ) {
		return false;
	}
	origin = frame[i].ToVec3();
	axis = frame[i].ToMat3();
	return true;
}

/*
================
GetArgString
================
*/
static const char *GetArgString( const idDict &args, const idDict *defArgs, const char *key ) {
	const char *s;

	s = args.GetString( key );
	if ( !s[0] && defArgs ) {
		s = defArgs->GetString( key );
	}
	return s;
}

/*
================
idGameEditLocal::AF_CreateMesh
================
*/
idRenderModel *idGameEditLocal::AF_CreateMesh( const idDict &args, idVec3 &meshOrigin, idMat3 &meshAxis, bool &poseIsSet ) {
	int i, jointNum;
	const idDeclAF *af;
	const idDeclAF_Body *fb;
	renderEntity_t ent;
	idVec3 origin, *bodyOrigin, *newBodyOrigin, *modifiedOrigin;
	idMat3 axis, *bodyAxis, *newBodyAxis, *modifiedAxis;
	declAFJointMod_t *jointMod;
	idAngles angles;
	const idDict *defArgs;
	const idKeyValue *arg;
	idStr name;
	jointTransformData_t data;
	const char *classname, *afName, *modelName;
	idRenderModel *md5;
	const idDeclModelDef *modelDef;
	const idMD5Anim *MD5anim;
	const idMD5Joint *MD5joint;
	const idMD5Joint *MD5joints;
	int numMD5joints;
	idJointMat *originalJoints;
	int parentNum;

	poseIsSet = false;
	meshOrigin.Zero();
	meshAxis.Identity();

	classname = args.GetString( "classname" );
	defArgs = gameLocal.FindEntityDefDict( classname );
	if ( !defArgs ) {
		gameLocal.Error( "Unknown classname '%s'", classname );
	}

	// get the articulated figure
	afName = GetArgString( args, defArgs, "articulatedFigure" );
	af = static_cast<const idDeclAF *>( declManager->FindType( DECL_AF, afName ) );
	if ( !af ) {
		return NULL;
	}

	// get the md5 model
	modelName = GetArgString( args, defArgs, "model" );
	modelDef = static_cast< const idDeclModelDef *>( declManager->FindType( DECL_MODELDEF, modelName, false ) );
	if ( !modelDef ) {
		return NULL;
	}

	// make sure model hasn't been purged
	if ( modelDef->ModelHandle() && !modelDef->ModelHandle()->IsLoaded() ) {
		modelDef->ModelHandle()->LoadModel();
	}

	// get the md5
	md5 = modelDef->ModelHandle();
	if ( !md5 || md5->IsDefaultModel() ) {
		return NULL;
	}

	// get the articulated figure pose anim
	int animNum = modelDef->GetAnim( "af_pose" );
	if ( !animNum ) {
		return NULL;
	}
	const idAnim *anim = modelDef->GetAnim( animNum );
	if ( !anim ) {
		return NULL;
	}
	MD5anim = anim->MD5Anim( 0 );
	MD5joints = md5->GetJoints();
	numMD5joints = md5->NumJoints();

	// setup a render entity
	memset( &ent, 0, sizeof( ent ) );
	ent.customSkin = modelDef->GetSkin();
	ent.bounds.Clear();
	ent.numJoints = numMD5joints;
	ent.joints = ( idJointMat * )_alloca16( ent.numJoints * sizeof( *ent.joints ) );

	// create animation from of the af_pose
	ANIM_CreateAnimFrame( md5, MD5anim, ent.numJoints, ent.joints, 1, modelDef->GetVisualOffset(), false );

	// buffers to store the initial origin and axis for each body
	bodyOrigin = (idVec3 *) _alloca16( af->bodies.Num() * sizeof( idVec3 ) );
	bodyAxis = (idMat3 *) _alloca16( af->bodies.Num() * sizeof( idMat3 ) );
	newBodyOrigin = (idVec3 *) _alloca16( af->bodies.Num() * sizeof( idVec3 ) );
	newBodyAxis = (idMat3 *) _alloca16( af->bodies.Num() * sizeof( idMat3 ) );

	// finish the AF positions
	data.ent = &ent;
	data.joints = MD5joints;
	af->Finish( GetJointTransform, ent.joints, &data );

	// get the initial origin and axis for each AF body
	for ( i = 0; i < af->bodies.Num(); i++ ) {
		fb = af->bodies[i];

		if ( fb->modelType == TRM_BONE ) {
			// axis of bone trace model
			axis[2] = fb->v2.ToVec3() - fb->v1.ToVec3();
			axis[2].Normalize();
			axis[2].NormalVectors( axis[0], axis[1] );
			axis[1] = -axis[1];
		} else {
			axis = fb->angles.ToMat3();
		}

		newBodyOrigin[i] = bodyOrigin[i] = fb->origin.ToVec3();
		newBodyAxis[i] = bodyAxis[i] = axis;
	}

	// get any new body transforms stored in the key/value pairs
	for ( arg = args.MatchPrefix( "body ", NULL ); arg; arg = args.MatchPrefix( "body ", arg ) ) {
		name = arg->GetKey();
		name.Strip( "body " );
		for ( i = 0; i < af->bodies.Num(); i++ ) {
			fb = af->bodies[i];
			if ( fb->name.Icmp( name ) == 0 ) {
				break;
			}
		}
		if ( i >= af->bodies.Num() ) {
			continue;
		}
		sscanf( arg->GetValue(), "%f %f %f %f %f %f", &origin.x, &origin.y, &origin.z, &angles.pitch, &angles.yaw, &angles.roll );

		if ( fb->jointName.Icmp( "origin" ) == 0 ) {
			meshAxis = bodyAxis[i].Transpose() * angles.ToMat3();
			meshOrigin = origin - bodyOrigin[i] * meshAxis;
			poseIsSet = true;
		} else {
			newBodyOrigin[i] = origin;
			newBodyAxis[i] = angles.ToMat3();
		}
	}

	// save the original joints
	originalJoints = ( idJointMat * )_alloca16( numMD5joints * sizeof( originalJoints[0] ) );
	memcpy( originalJoints, ent.joints, numMD5joints * sizeof( originalJoints[0] ) );

	// buffer to store the joint mods
	jointMod = (declAFJointMod_t *) _alloca16( numMD5joints * sizeof( declAFJointMod_t ) );
	memset( jointMod, -1, numMD5joints * sizeof( declAFJointMod_t ) );
	modifiedOrigin = (idVec3 *) _alloca16( numMD5joints * sizeof( idVec3 ) );
	memset( modifiedOrigin, 0, numMD5joints * sizeof( idVec3 ) );
	modifiedAxis = (idMat3 *) _alloca16( numMD5joints * sizeof( idMat3 ) );
	memset( modifiedAxis, 0, numMD5joints * sizeof( idMat3 ) );

	// get all the joint modifications
	for ( i = 0; i < af->bodies.Num(); i++ ) {
		fb = af->bodies[i];

		if ( fb->jointName.Icmp( "origin" ) == 0 ) {
			continue;
		}

		for ( jointNum = 0; jointNum < numMD5joints; jointNum++ ) {
			if ( MD5joints[jointNum].name.Icmp( fb->jointName ) == 0 ) {
				break;
			}
		}

		if ( jointNum >= 0 && jointNum < ent.numJoints ) {
			jointMod[ jointNum ] = fb->jointMod;
			modifiedAxis[ jointNum ] = ( bodyAxis[i] * originalJoints[jointNum].ToMat3().Transpose() ).Transpose() * ( newBodyAxis[i] * meshAxis.Transpose() );
			// FIXME: calculate correct modifiedOrigin
			modifiedOrigin[ jointNum ] = originalJoints[ jointNum ].ToVec3();
		}
	}

	// apply joint modifications to the skeleton
	MD5joint = MD5joints + 1;
	for( i = 1; i < numMD5joints; i++, MD5joint++ ) {

		parentNum = MD5joint->parent - MD5joints;
		idMat3 parentAxis = originalJoints[ parentNum ].ToMat3();
		idMat3 localm = originalJoints[i].ToMat3() * parentAxis.Transpose();
		idVec3 localt = ( originalJoints[i].ToVec3() - originalJoints[ parentNum ].ToVec3() ) * parentAxis.Transpose();

		switch( jointMod[i] ) {
			case DECLAF_JOINTMOD_ORIGIN: {
				ent.joints[ i ].SetRotation( localm * ent.joints[ parentNum ].ToMat3() );
				ent.joints[ i ].SetTranslation( modifiedOrigin[ i ] );
				break;
			}
			case DECLAF_JOINTMOD_AXIS: {
				ent.joints[ i ].SetRotation( modifiedAxis[ i ] );
				ent.joints[ i ].SetTranslation( ent.joints[ parentNum ].ToVec3() + localt * ent.joints[ parentNum ].ToMat3() );
				break;
			}
			case DECLAF_JOINTMOD_BOTH: {
				ent.joints[ i ].SetRotation( modifiedAxis[ i ] );
				ent.joints[ i ].SetTranslation( modifiedOrigin[ i ] );
				break;
			}
			default: {
				ent.joints[ i ].SetRotation( localm * ent.joints[ parentNum ].ToMat3() );
				ent.joints[ i ].SetTranslation( ent.joints[ parentNum ].ToVec3() + localt * ent.joints[ parentNum ].ToMat3() );
				break;
			}
		}
	}

	// instantiate a mesh using the joint information from the render entity
	return md5->InstantiateDynamicModel( &ent, NULL, NULL );
}

/***********************************************************************

	Util functions

***********************************************************************/

/*
=====================
ANIM_GetModelDefFromEntityDef
=====================
*/
const idDeclModelDef *ANIM_GetModelDefFromEntityDef( const idDict *args ) {
	const idDeclModelDef *modelDef;

	idStr name = args->GetString( "model" );
	modelDef = static_cast<const idDeclModelDef *>( declManager->FindType( DECL_MODELDEF, name, false ) );
	if ( modelDef && modelDef->ModelHandle() ) {
		return modelDef;
	}

	return NULL;
}

/*
=====================
idGameEditLocal::ANIM_GetModelFromEntityDef
=====================
*/
idRenderModel *idGameEditLocal::ANIM_GetModelFromEntityDef( const idDict *args ) {
	idRenderModel *model;
	const idDeclModelDef *modelDef;

	model = NULL;

	idStr name = args->GetString( "model" );
	modelDef = static_cast<const idDeclModelDef *>( declManager->FindType( DECL_MODELDEF, name, false ) );
	if ( modelDef ) {
		model = modelDef->ModelHandle();
	}

	if ( !model ) {
		model = renderModelManager->FindModel( name );
	}

	if ( model && model->IsDefaultModel() ) {
		return NULL;
	}

	return model;
}

/*
=====================
idGameEditLocal::ANIM_GetModelFromEntityDef
=====================
*/
idRenderModel *idGameEditLocal::ANIM_GetModelFromEntityDef( const char *classname ) {
	const idDict *args;

	args = gameLocal.FindEntityDefDict( classname, false );
	if ( !args ) {
		return NULL;
	}

	return ANIM_GetModelFromEntityDef( args );
}

/*
=====================
idGameEditLocal::ANIM_GetModelOffsetFromEntityDef
=====================
*/
const idVec3 &idGameEditLocal::ANIM_GetModelOffsetFromEntityDef( const char *classname ) {
	const idDict *args;
	const idDeclModelDef *modelDef;

	args = gameLocal.FindEntityDefDict( classname, false );
	if ( !args ) {
		return vec3_origin;
	}

	modelDef = ANIM_GetModelDefFromEntityDef( args );
	if ( !modelDef ) {
		return vec3_origin;
	}

	return modelDef->GetVisualOffset();
}

/*
=====================
idGameEditLocal::ANIM_GetModelFromName
=====================
*/
idRenderModel *idGameEditLocal::ANIM_GetModelFromName( const char *modelName ) {
	const idDeclModelDef *modelDef;
	idRenderModel *model;

	model = NULL;
	modelDef = static_cast<const idDeclModelDef *>( declManager->FindType( DECL_MODELDEF, modelName, false ) );
	if ( modelDef ) {
		model = modelDef->ModelHandle();
	}
	if ( !model ) {
		model = renderModelManager->FindModel( modelName );
	}
	return model;
}

/*
=====================
idGameEditLocal::ANIM_GetAnimFromEntityDef
=====================
*/
const idMD5Anim *idGameEditLocal::ANIM_GetAnimFromEntityDef( const char *classname, const char *animname ) {
	const idDict *args;
	const idMD5Anim *md5anim;
	const idAnim *anim;
	int	animNum;
	const char	*modelname;
	const idDeclModelDef *modelDef;

	args = gameLocal.FindEntityDefDict( classname, false );
	if ( !args ) {
		return NULL;
	}

	md5anim = NULL;
	modelname = args->GetString( "model" );
	modelDef = static_cast<const idDeclModelDef *>( declManager->FindType( DECL_MODELDEF, modelname, false ) );
	if ( modelDef ) {
		animNum = modelDef->GetAnim( animname );
		if ( animNum ) {
			anim = modelDef->GetAnim( animNum );
			if ( anim ) {
				md5anim = anim->MD5Anim( 0 );
			}
		}
	}
	return md5anim;
}

// HUMANHEAD pdm: Allow editor to query a dictionary to get animation to allow entitydefs that are not completely predefined
// to display animated models.  (like func_animates)
const idMD5Anim *idGameEditLocal::ANIM_GetAnimFromArgs( const idDict *args, const char *animname ) {
	const idMD5Anim *md5anim;
	const idAnim *anim;
	int	animNum;
	const char	*modelname;
	const idDeclModelDef *modelDef;

	md5anim = NULL;
	modelname = args->GetString( "model" );
	modelDef = static_cast<const idDeclModelDef *>( declManager->FindType( DECL_MODELDEF, modelname, false ) );
	if ( modelDef ) {
		animNum = modelDef->GetAnim( animname );
		if ( animNum ) {
			anim = modelDef->GetAnim( animNum );
			if ( anim ) {
				md5anim = anim->MD5Anim( 0 );
			}
		}
	}
	return md5anim;
}

/*
=====================
idGameEditLocal::ANIM_GetNumAnimsFromEntityDef
=====================
*/
int idGameEditLocal::ANIM_GetNumAnimsFromEntityDef( const idDict *args ) {
	const char *modelname;
	const idDeclModelDef *modelDef;

	modelname = args->GetString( "model" );
	modelDef = static_cast<const idDeclModelDef *>( declManager->FindType( DECL_MODELDEF, modelname, false ) );
	if ( modelDef ) {
		return modelDef->NumAnims();
	}
	return 0;
}

/*
=====================
idGameEditLocal::ANIM_GetAnimNameFromEntityDef
=====================
*/
const char *idGameEditLocal::ANIM_GetAnimNameFromEntityDef( const idDict *args, int animNum ) {
	const char *modelname;
	const idDeclModelDef *modelDef;

	modelname = args->GetString( "model" );
	modelDef = static_cast<const idDeclModelDef *>( declManager->FindType( DECL_MODELDEF, modelname, false ) );
	if ( modelDef ) {
		const idAnim* anim = modelDef->GetAnim( animNum );
		if ( anim ) {
			return anim->FullName();
		}
	}
	return "";
}

/*
=====================
idGameEditLocal::ANIM_GetAnim
=====================
*/
const idMD5Anim *idGameEditLocal::ANIM_GetAnim( const char *fileName ) {
	return animationLib.GetAnim( fileName );
}

/*
=====================
idGameEditLocal::ANIM_GetLength
=====================
*/
int	idGameEditLocal::ANIM_GetLength( const idMD5Anim *anim ) {
	if ( !anim ) {
		return 0;
	}
	return anim->Length();
}

/*
=====================
idGameEditLocal::ANIM_GetNumFrames
=====================
*/
int idGameEditLocal::ANIM_GetNumFrames( const idMD5Anim *anim ) {
	if ( !anim ) {
		return 0;
	}
	return anim->NumFrames();
}

/*
=====================
idGameEditLocal::ANIM_CreateAnimFrame
=====================
*/
void idGameEditLocal::ANIM_CreateAnimFrame( const idRenderModel *model, const idMD5Anim *anim, int numJoints, idJointMat *joints, int time, const idVec3 &offset, bool remove_origin_offset ) {
	int					i;
	frameBlend_t		frame;
	const idMD5Joint	*md5joints;
	int					*index;

	if ( !model || model->IsDefaultModel() || !anim ) {
		return;
	}

	if ( numJoints != model->NumJoints() ) {
		gameLocal.Error( "ANIM_CreateAnimFrame: different # of joints in renderEntity_t than in model (%s)", model->Name() );
	}

	if ( !model->NumJoints() ) {
		// FIXME: Print out a warning?
		return;
	}

	if ( !joints ) {
		gameLocal.Error( "ANIM_CreateAnimFrame: NULL joint frame pointer on model (%s)", model->Name() );
	}

	if ( numJoints != anim->NumJoints() ) {
		gameLocal.Warning( "Model '%s' has different # of joints than anim '%s'", model->Name(), anim->Name() );
		for( i = 0; i < numJoints; i++ ) {
			joints[i].SetRotation( mat3_identity );
			joints[i].SetTranslation( offset );
		}
		return;
	}

	// create index for all joints
	index = ( int * )_alloca16( numJoints * sizeof( int ) );
	for ( i = 0; i < numJoints; i++ ) {
		index[i] = i;
	}

	// create the frame
	anim->ConvertTimeToFrame( time, 1, frame );
	idJointQuat *jointFrame = ( idJointQuat * )_alloca16( numJoints * sizeof( *jointFrame ) );
	anim->GetInterpolatedFrame( frame, jointFrame, index, numJoints );

	// convert joint quaternions to joint matrices
	SIMDProcessor->ConvertJointQuatsToJointMats( joints, jointFrame, numJoints );

	// first joint is always root of entire hierarchy
	if ( remove_origin_offset ) {
		joints[0].SetTranslation( offset );
	} else {
		joints[0].SetTranslation( joints[0].ToVec3() + offset );
	}

	// transform the children
	md5joints = model->GetJoints();
	for( i = 1; i < numJoints; i++ ) {
		joints[i] *= joints[ md5joints[i].parent - md5joints ];
	}
}

/*
=====================
idGameEditLocal::ANIM_CreateMeshForAnim
=====================
*/
idRenderModel *idGameEditLocal::ANIM_CreateMeshForAnim( idRenderModel *model, const char *classname, const char *animname, int frame, bool remove_origin_offset ) {
	renderEntity_t			ent;
	const idDict			*args;
	const char				*temp;
	idRenderModel			*newmodel;
	const idMD5Anim			*md5anim;
	idStr					filename;
	idStr					extension;
	const idAnim			*anim;
	int						animNum;
	idVec3					offset;
	const idDeclModelDef	*modelDef;

	if ( !model || model->IsDefaultModel() ) {
		return NULL;
	}

	args = gameLocal.FindEntityDefDict( classname, false );
	if ( !args ) {
		return NULL;
	}

	memset( &ent, 0, sizeof( ent ) );

	ent.bounds.Clear();
	ent.suppressSurfaceInViewID = 0;

	modelDef = ANIM_GetModelDefFromEntityDef( args );
	if ( modelDef ) {
		animNum = modelDef->GetAnim( animname );
		if ( !animNum ) {
			return NULL;
		}
		anim = modelDef->GetAnim( animNum );
		if ( !anim ) {
			return NULL;
		}
		md5anim = anim->MD5Anim( 0 );
		ent.customSkin = modelDef->GetDefaultSkin();
		offset = modelDef->GetVisualOffset();
	} else {
		filename = animname;
		filename.ExtractFileExtension( extension );
		if ( !extension.Length() ) {
			animname = args->GetString( va( "anim %s", animname ) );
		}

		md5anim = animationLib.GetAnim( animname );
		offset.Zero();
	}

	if ( !md5anim ) {
		return NULL;
	}

	temp = args->GetString( "skin", "" );
	if ( temp[ 0 ] ) {
		ent.customSkin = declManager->FindSkin( temp );
	}

	ent.numJoints = model->NumJoints();
	ent.joints = ( idJointMat * )Mem_Alloc16( ent.numJoints * sizeof( *ent.joints ) );

	ANIM_CreateAnimFrame( model, md5anim, ent.numJoints, ent.joints, FRAME2MS( frame ), offset, remove_origin_offset );

	newmodel = model->InstantiateDynamicModel( &ent, NULL, NULL );

	Mem_Free16( ent.joints );
	ent.joints = NULL;

	return newmodel;
}

/*
================
idGameEditLocal::ParseSpawnArgsToRenderEntity

parse the static model parameters
this is the canonical renderEntity parm parsing,
which should be used by dmap and the editor
================
*/
void idGameEditLocal::ParseSpawnArgsToRenderEntity( const idDict *args, renderEntity_t *renderEntity ) {
	int			i;
	const char	*temp;
	idVec3		color;
	float		angle;
	const idDeclModelDef *modelDef;

	memset( renderEntity, 0, sizeof( *renderEntity ) );

	temp = args->GetString( "model" );

	modelDef = NULL;
	if ( temp[0] != '\0' ) {
		modelDef = static_cast<const idDeclModelDef *>( declManager->FindType( DECL_MODELDEF, temp, false ) );
		if ( modelDef ) {
			renderEntity->hModel = modelDef->ModelHandle();
		}
		if ( !renderEntity->hModel ) {
			renderEntity->hModel = renderModelManager->FindModel( temp );
		}
	}
	if ( renderEntity->hModel ) {
		renderEntity->bounds = renderEntity->hModel->Bounds( renderEntity );
	} else {
		renderEntity->bounds.Zero();
	}

	temp = args->GetString( "skin" );
	if ( temp[0] != '\0' ) {
		renderEntity->customSkin = declManager->FindSkin( temp );
	} else if ( modelDef ) {
		renderEntity->customSkin = modelDef->GetDefaultSkin();
	}

	temp = args->GetString( "shader" );
	if ( temp[0] != '\0' ) {
		renderEntity->customShader = declManager->FindMaterial( temp );
	}

	args->GetVector( "origin", "0 0 0", renderEntity->origin );

	// get the rotation matrix in either full form, or single angle form
	if ( !args->GetMatrix( "rotation", "1 0 0 0 1 0 0 0 1", renderEntity->axis ) ) {
		//HUMANHEAD: aob - so editor 'up' and 'down' buttons work on entities
		idAngles angles( ang_zero );
		angle = args->GetFloat( "angle" );
		if( angle == -1 ) {
			angles[ 0 ] = -90.0f;
		}
		else if( angle == -2 ) {
			angles[ 0 ] = 90.0f;
		}
		else {
			angles[ 0 ] = args->GetFloat( "pitch" );
			angles[ 1 ] = angle;
			angles[ 2 ] = args->GetFloat( "roll" );
		}
		//HUMANHEAD END
		renderEntity->axis = angles.ToMat3();
	}

	renderEntity->referenceSound = NULL;

	// get shader parms
	args->GetVector( "_color", "1 1 1", color );
	renderEntity->shaderParms[ SHADERPARM_RED ]		= color[0];
	renderEntity->shaderParms[ SHADERPARM_GREEN ]	= color[1];
	renderEntity->shaderParms[ SHADERPARM_BLUE ]	= color[2];
	renderEntity->shaderParms[ 3 ]					= args->GetFloat( "shaderParm3", "1" );
	renderEntity->shaderParms[ 4 ]					= args->GetFloat( "shaderParm4", "0" );
	renderEntity->shaderParms[ 5 ]					= args->GetFloat( "shaderParm5", "0" );
	renderEntity->shaderParms[ 6 ]					= args->GetFloat( "shaderParm6", "0" );
	renderEntity->shaderParms[ 7 ]					= args->GetFloat( "shaderParm7", "0" );
	renderEntity->shaderParms[ 8 ]					= args->GetFloat( "shaderParm8", "0" );
	renderEntity->shaderParms[ 9 ]					= args->GetFloat( "shaderParm9", "0" );
	renderEntity->shaderParms[ 10 ]					= args->GetFloat( "shaderParm10", "0" );
	renderEntity->shaderParms[ 11 ]					= args->GetFloat( "shaderParm11", "0" );

	// HUMANHEAD pdm: added scale available at spawn time (should be visible in editor, but isn't)
	renderEntity->shaderParms[ 12 ]					= args->GetFloat( "shaderParm12", "0" );
	renderEntity->onlyVisibleInSpirit				= args->GetBool( "onlyVisibleInSpirit" );
	renderEntity->onlyInvisibleInSpirit				= args->GetBool( "onlyInvisibleInSpirit" );	// tmj
	renderEntity->lowSkippable						= args->GetBool( "lowSkippable" );	// bjk
	if(renderEntity->onlyVisibleInSpirit && renderEntity->onlyInvisibleInSpirit) {
		gameLocal.Warning( "Entity is both visible and invisible in spiritwalk: %s", args->GetString( "name" ));
	}

	if (args->FindKey("deformType")) {
		int deformType = args->GetInt("deformType");
		float parm1 = args->GetFloat("deformParm1");
		float parm2 = args->GetFloat("deformParm2");
		SetDeformationOnRenderEntity(renderEntity, deformType, parm1, parm2);
	}

	if (args->FindKey("scale")) {
		renderEntity->shaderParms[SHADERPARM_ANY_DEFORM] = DEFORMTYPE_SCALE;
		renderEntity->shaderParms[SHADERPARM_ANY_DEFORM_PARM1] = args->GetFloat("scale", "0");
	}
	// HUMANHEAD END

	// check noDynamicInteractions flag
	renderEntity->noDynamicInteractions = args->GetBool( "noDynamicInteractions" );

	// check noshadows flag
	renderEntity->noShadow = args->GetBool( "noshadows" );

	// check noselfshadows flag
	renderEntity->noSelfShadow = args->GetBool( "noselfshadows" );

	// init any guis, including entity-specific states
	for( i = 0; i < MAX_RENDERENTITY_GUI; i++ ) {
		temp = args->GetString( i == 0 ? "gui" : va( "gui%d", i + 1 ) );
		if ( temp[ 0 ] != '\0' ) {
			AddRenderGui( temp, &renderEntity->gui[ i ], args );
		}
	}
}

/*
================
idGameEditLocal::ParseSpawnArgsToRefSound

parse the sound parameters
this is the canonical refSound parm parsing,
which should be used by dmap and the editor
================
*/
void idGameEditLocal::ParseSpawnArgsToRefSound( const idDict *args, refSound_t *refSound ) {
	const char	*temp;

	memset( refSound, 0, sizeof( *refSound ) );

	refSound->parms.minDistance = args->GetFloat( "s_mindistance" );
	refSound->parms.maxDistance = args->GetFloat( "s_maxdistance" );
	refSound->parms.volume = args->GetFloat( "s_volume" );
	refSound->parms.shakes = args->GetFloat( "s_shakes" );

	args->GetVector( "origin", "0 0 0", refSound->origin );

	refSound->referenceSound  = NULL;

	// if a diversity is not specified, every sound start will make
	// a random one.  Specifying diversity is usefull to make multiple
	// lights all share the same buzz sound offset, for instance.
	refSound->diversity = args->GetFloat( "s_diversity", "-1" );
	refSound->waitfortrigger = args->GetBool( "s_waitfortrigger" );

	if ( args->GetBool( "s_omni" ) ) {
		refSound->parms.soundShaderFlags |= SSF_OMNIDIRECTIONAL;
	}
	if ( args->GetBool( "s_looping" ) ) {
		refSound->parms.soundShaderFlags |= SSF_LOOPING;
	}
	if ( args->GetBool( "s_occlusion" ) ) {
		refSound->parms.soundShaderFlags |= SSF_NO_OCCLUSION;
	}
	if ( args->GetBool( "s_global" ) ) {
		refSound->parms.soundShaderFlags |= SSF_GLOBAL;
	}
	if ( args->GetBool( "s_unclamped" ) ) {
		refSound->parms.soundShaderFlags |= SSF_UNCLAMPED;
	}
	refSound->parms.soundClass = args->GetInt( "s_soundClass" );

	temp = args->GetString( "s_shader" );
	if ( temp[0] != '\0' ) {
		refSound->shader = declManager->FindSound( temp );
	}
}

/*
================
idGameEditLocal::ParseSpawnArgsToRenderLight

parse the light parameters
this is the canonical renderLight parm parsing,
which should be used by dmap and the editor
================
*/
void idGameEditLocal::ParseSpawnArgsToRenderLight( const idDict *args, renderLight_t *renderLight ) {
	bool	gotTarget, gotUp, gotRight;
	const char	*texture;
	idVec3	color;

	memset( renderLight, 0, sizeof( *renderLight ) );

	if (!args->GetVector("light_origin", "", renderLight->origin)) {
		args->GetVector( "origin", "", renderLight->origin );
	}

	gotTarget = args->GetVector( "light_target", "", renderLight->target );
	gotUp = args->GetVector( "light_up", "", renderLight->up );
	gotRight = args->GetVector( "light_right", "", renderLight->right );
	args->GetVector( "light_start", "0 0 0", renderLight->start );
	if ( !args->GetVector( "light_end", "", renderLight->end ) ) {
		renderLight->end = renderLight->target;
	}

	// we should have all of the target/right/up or none of them
	if ( ( gotTarget || gotUp || gotRight ) != ( gotTarget && gotUp && gotRight ) ) {
		gameLocal.Printf( "Light at (%f,%f,%f) has bad target info\n",
			renderLight->origin[0], renderLight->origin[1], renderLight->origin[2] );
		return;
	}

	if ( !gotTarget ) {
		renderLight->pointLight = true;

		// allow an optional relative center of light and shadow offset
		args->GetVector( "light_center", "0 0 0", renderLight->lightCenter );

		// create a point light
		if (!args->GetVector( "light_radius", "300 300 300", renderLight->lightRadius ) ) {
			float radius;

			args->GetFloat( "light", "300", radius );
			renderLight->lightRadius[0] = renderLight->lightRadius[1] = renderLight->lightRadius[2] = radius;
		}
	}

	// get the rotation matrix in either full form, or single angle form
	idAngles angles;
	idMat3 mat;
	if ( !args->GetMatrix( "light_rotation", "1 0 0 0 1 0 0 0 1", mat ) ) {
		if ( !args->GetMatrix( "rotation", "1 0 0 0 1 0 0 0 1", mat ) ) {
			args->GetFloat( "angle", "0", angles[ 1 ] );
			angles[ 0 ] = 0;
			angles[ 1 ] = idMath::AngleNormalize360( angles[ 1 ] );
			angles[ 2 ] = 0;
			mat = angles.ToMat3();
		}
	}

	// fix degenerate identity matrices
	mat[0].FixDegenerateNormal();
	mat[1].FixDegenerateNormal();
	mat[2].FixDegenerateNormal();

	renderLight->axis = mat;

	// check for other attributes
	args->GetVector( "_color", "1 1 1", color );
	renderLight->shaderParms[ SHADERPARM_RED ]		= color[0];
	renderLight->shaderParms[ SHADERPARM_GREEN ]	= color[1];
	renderLight->shaderParms[ SHADERPARM_BLUE ]		= color[2];
	args->GetFloat( "shaderParm3", "1", renderLight->shaderParms[ SHADERPARM_TIMESCALE ] );
	if ( !args->GetFloat( "shaderParm4", "0", renderLight->shaderParms[ SHADERPARM_TIMEOFFSET ] ) ) {
		// offset the start time of the shader to sync it to the game time
		renderLight->shaderParms[ SHADERPARM_TIMEOFFSET ] = -MS2SEC( gameLocal.time );
	}

	args->GetFloat( "shaderParm5", "0", renderLight->shaderParms[5] );
	args->GetFloat( "shaderParm6", "0", renderLight->shaderParms[6] );
	args->GetFloat( "shaderParm7", "0", renderLight->shaderParms[ SHADERPARM_MODE ] );
	args->GetBool( "noshadows", "0", renderLight->noShadows );
	args->GetBool( "nospecular", "0", renderLight->noSpecular );
	args->GetBool( "lowSkippable", "0", renderLight->lowSkippable );	//HUMANHEAD bjk
	args->GetBool( "parallel", "0", renderLight->parallel );

	args->GetString( "texture", "lights/squarelight1", &texture );
	// allow this to be NULL
	renderLight->shader = declManager->FindMaterial( texture, false );
}

/***********************************************************************

  Debugger

***********************************************************************/

bool idGameEditLocal::IsLineCode( const char *filename, int linenumber ) const {
	idStr fileStr;
	idProgram *program = &gameLocal.program;
	for ( int i = 0; i < program->NumStatements( ); i++ ) 	{
		fileStr = program->GetFilename( program->GetStatement( i ).file );
		fileStr.BackSlashesToSlashes( );

		if ( strcmp( filename, fileStr.c_str( ) ) == 0
			&& program->GetStatement( i ).linenumber == linenumber
			) {
			return true;
		}
	}
	return false;
}

void idGameEditLocal::GetLoadedScripts( idStrList **result ) {
	(*result)->Clear();
	idProgram* program = &gameLocal.program;

	for ( int i = 0; i < program->NumFilenames(); i++ ) {
		(*result)->AddUnique( idStr( program->GetFilename( i ) ) );
	}
}

void idGameEditLocal::MSG_WriteScriptList( idBitMsg *msg ) {
	idProgram* program = &gameLocal.program;

	msg->WriteInt( program->NumFilenames() );
	for ( int i = 0; i < program->NumFilenames(); i++ ) {
		idStr file = program->GetFilename(i);
		//fix this. it seams that scripts triggered by the runtime are stored with a wrong path
		//the use // instead of '\'
		file.BackSlashesToSlashes();
		msg->WriteString(file);
	}
}

const char *idGameEditLocal::GetFilenameForStatement( idProgram *program, int index ) const {
	return program->GetFilenameForStatement( index );
}

int idGameEditLocal::GetLineNumberForStatement( idProgram *program, int index ) const {
	return program->GetLineNumberForStatement( index );
}

bool idGameEditLocal::CheckForBreakPointHit( const idInterpreter *interpreter, const function_t *function1, const function_t *function2, int depth ) const {
	return ( ( interpreter->GetCurrentFunction() == function1 ||
			   interpreter->GetCurrentFunction() == function2)&&
			 ( interpreter->GetCallstackDepth()  <= depth) );
}

bool idGameEditLocal::ReturnedFromFunction( const idProgram *program, const idInterpreter *interpreter, int index ) const {

	return ( const_cast<idProgram *>( program )->GetStatement( index ).op == OP_RETURN && interpreter->GetCallstackDepth() <= 1 );
}

bool idGameEditLocal::GetRegisterValue(const idInterpreter* interpreter, const char* name, idStr& out, int scopeDepth) const
{
	return const_cast<idInterpreter*>(interpreter)->GetRegisterValue(name, out, scopeDepth);
}

const idThread *idGameEditLocal::GetThread( const idInterpreter *interpreter ) const {
	return interpreter->GetThread();
}

void idGameEditLocal::MSG_WriteCallstackFunc( idBitMsg *msg, const prstack_t *stack, const idProgram *program, int instructionPtr ) {
	const statement_t*	st;
	const function_t*	func;

	func  = stack->f;

	// If the function is unknown then just fill in with default data.
	if ( !func ) {
		msg->WriteString ( "<UNKNOWN>" );
		msg->WriteString ( "<UNKNOWN>" );
		msg->WriteInt ( 0 );
		return;
	} else {
		msg->WriteString ( va("%s(  )", func->Name() ) );
	}

	if (stack->s == -1) //this is a fake stack created by debugger, use intruction pointer for retrieval.
		st = &const_cast<idProgram*>( program )->GetStatement( instructionPtr );
	else // Use the calling statement as the filename and linenumber where the call was made from
		st = &const_cast<idProgram*>( program )->GetStatement ( stack->s );

	if ( st ) {
		idStr qpath = const_cast<idProgram*>( program )->GetFilename( st->file );
		if ( idStr::FindChar( qpath, ':' ) != -1 ) {
			qpath = fileSystem->OSPathToRelativePath( qpath.c_str() );
		}
		qpath.BackSlashesToSlashes ( );
		msg->WriteString( qpath );
		msg->WriteInt( st->linenumber );
	} else {
		msg->WriteString ( "<UNKNOWN>" );
		msg->WriteInt ( 0 );
	}
}

void idGameEditLocal::MSG_WriteInterpreterInfo( idBitMsg *msg, const idInterpreter *interpreter, const idProgram *program, int instructionPtr ) {
	int			i;
	prstack_s	temp;

	msg->WriteShort( (int)interpreter->GetCallstackDepth() );

	// write out the current function
	temp.f = interpreter->GetCurrentFunction();
	temp.s = -1;
	temp.stackbase = 0;
	MSG_WriteCallstackFunc( msg, &temp, program, instructionPtr );

	// Run through all of the callstack and write each to the msg
	for ( i = interpreter->GetCallstackDepth() - 1; i > 0; i-- ) {
		MSG_WriteCallstackFunc( msg, interpreter->GetCallstack() + i, program, instructionPtr );
	}
}

int idGameEditLocal::GetInterpreterCallStackDepth( const idInterpreter *interpreter ) {
	return interpreter->GetCallstackDepth();
}

const function_t *idGameEditLocal::GetInterpreterCallStackFunction( const idInterpreter *interpreter, int stackDepth/* = -1*/ ) {
	return interpreter->GetCallstack()[ stackDepth > -1 ? stackDepth : interpreter->GetCallstackDepth() ].f;
}

int idGameEditLocal::ThreadGetNum( const idThread *thread ) const {
	return const_cast<idThread *>( thread )->GetThreadNum();
}

const char* idGameEditLocal::ThreadGetName( const idThread *thread ) const {
	return const_cast<idThread *>( thread )->GetThreadName();
}

int	idGameEditLocal::GetTotalScriptThreads( void ) const {
	return idThread::GetThreads().Num();
}

const idThread* idGameEditLocal::GetThreadByIndex( int index ) const {
	return idThread::GetThreads()[index];
}

bool idGameEditLocal::ThreadIsDoneProcessing( const idThread *thread ) const {
	return const_cast<idThread *>( thread )->IsDoneProcessing();
}

bool idGameEditLocal::ThreadIsWaiting( const idThread *thread ) const {
	return const_cast<idThread *>( thread )->IsWaiting();
}

bool idGameEditLocal::ThreadIsDying( const idThread *thread ) const {
	return const_cast<idThread *>( thread )->IsDying();
}

void idGameEditLocal::MSG_WriteThreadInfo( idBitMsg *msg, const idThread *thread, const idInterpreter *interpreter ) {
	msg->WriteString( const_cast<idThread *>( thread )->GetThreadName() );
	msg->WriteInt( const_cast<idThread *>( thread )->GetThreadNum() );

	msg->WriteBits( (int)( thread == interpreter->GetThread() ), 1 );
	msg->WriteBits( (int)const_cast<idThread *>( thread )->IsDoneProcessing(), 1 );
	msg->WriteBits( (int)const_cast<idThread *>( thread )->IsWaiting(), 1 );
	msg->WriteBits( (int)const_cast<idThread *>( thread )->IsDying(), 1 );
}