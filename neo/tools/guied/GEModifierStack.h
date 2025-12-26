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
#ifndef GEMODIFIERSTACK_H_
#define GEMODIFIERSTACK_H_

#ifndef GEMODIFIER_H_
#include "GEModifier.h"
#endif

class rvGEModifierStack {
public:

	rvGEModifierStack( );
	~rvGEModifierStack( );

	void			BlockNextMerge();

	bool			Append( rvGEModifier* modifier );
	bool			Undo();
	bool			Redo();

	void			Reset();

	bool			CanUndo();
	bool			CanRedo();

	rvGEModifier	* GetUndoModifier();
	rvGEModifier	* GetRedoModifier();

protected:

	idList<rvGEModifier *>	mModifiers;
	int						mCurrentModifier;
	bool					mMergeBlock;
};

ID_INLINE bool rvGEModifierStack::CanRedo() {
	return mCurrentModifier < mModifiers.Num() - 1;
}

ID_INLINE bool rvGEModifierStack::CanUndo() {
	return mCurrentModifier >= 0;
}

ID_INLINE void rvGEModifierStack::BlockNextMerge() {
	mMergeBlock = true;
}

ID_INLINE rvGEModifier * rvGEModifierStack::GetUndoModifier() {
	assert( CanUndo( ) );
	return mModifiers[mCurrentModifier];
}

ID_INLINE rvGEModifier * rvGEModifierStack::GetRedoModifier() {
	assert( CanRedo( ) );
	return mModifiers[mCurrentModifier + 1];
}

#endif
