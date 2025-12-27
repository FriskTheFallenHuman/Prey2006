/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.
Copyright (C) 2015 Daniel Gibson
Copyright (C) 2020-2023 Robert Beckebans

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

#ifndef NEO_TOOLS_EDITORS_SOUNDEDITOR_H_
#define NEO_TOOLS_EDITORS_SOUNDEDITOR_H_

class idEntity;

namespace ImGuiTools
{

class SoundEditor
{
private:
	bool				isShown;
	bool				showTool;
	idStr				title;

	// current selection / values
	idStr				strShader;
	idStr				playSound;
	float				fVolume;
	float				fMin;
	float				fMax;
	bool				bPlay;
	bool				bTriggered;
	bool				bOmni;
	idStr				strGroup;
	bool				bGroupOnly;
	bool				bOcclusion;
	float				leadThrough;
	bool				plain;
	float				random;
	float				wait;
	float				shakes;
	bool				looping;
	bool				unclamped;

	idList<idStr>		soundFiles; // shader names and wave file names in separate lists
	idList<idStr>		soundShaders;
	idList<const char*> groupsList;
	idList<const char*> speakersList;
	idList<idStr>		inUseSounds;

	// UI state
	int					selectedGroupIndex;
	int					selectedSpeakerIndex;
	char				shaderBuf[512];
	char				groupBuf[512];
	char				volumeBuf[32];
	char				newSpeakerBuf[256];
	bool				autoRefresh;
	bool				didAutoRefresh;
	// drop speaker popup state
	char				dropErrorBuf[256];
	bool				showDropError;

	// current speaker entity being edited
	idEntity*			speakerEntity;
	idStr				entityName;
	idVec3				entityPos;

	SoundEditor();

	void Init( const idDict* dict, idEntity* speaker );

	void Exit( void );
	void AddSounds( void );
	void AddWaves( void );
	void AddGroups( void );
	void AddSpeakers( void );

public:
	static SoundEditor& Instance();
	static void			ReInit( const idDict* dict, idEntity* speaker );

	inline void			ShowIt( bool show ) { isShown = show; }
	inline bool			IsShown() const { return isShown; }

	void				Draw( void );
	void				Reset( void );
	void				Set( const idDict* dict );
	void				ApplyChanges( bool volumeOnly );
	bool				DropSpeaker( const char* name = NULL );
	void				DeleteSelectedSpeakers();
	void				AddInUseSounds();
};

} // namespace ImGuiTools

#endif // NEO_TOOLS_EDITORS_SOUNDEDITOR_H_
