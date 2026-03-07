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

#ifndef __SOUNDHARDWARE_H__
#define __SOUNDHARDWARE_H__

/*
===============================================================================

	SOUND HARDWARE

===============================================================================
*/

class idSoundHardware {
public:
						idSoundHardware() {
							vuMeterRMS = NULL;
							vuMeterPeak = NULL;

							outputChannels = 0;
							channelMask = 0;

							lastResetTime = 0;
						}

	virtual				~idSoundHardware() {}

	virtual void		Init() = 0;
	virtual void		Shutdown() = 0;
	virtual void		ShutdownReverbSystem() = 0;

	virtual void		Update() = 0;
	virtual void		UpdateEAXEffect( idSoundEffect * effect ) = 0;

	virtual idSoundVoice *	AllocateVoice( const idSoundSample * leadinSample, const idSoundSample * loopingSample, const int channel ) = 0;
	virtual void		FreeVoice( idSoundVoice * voice ) = 0;

	virtual int			GetNumZombieVoices() const  = 0;
	virtual int			GetNumFreeVoices() const = 0;

	virtual bool		IsReverbSupported() = 0;
	virtual bool		ParseEAXEffects( idLexer & src, idToken name, idToken token, idSoundEffect * effect ) = 0;

protected:
	friend class		idSoundSample;
	friend class		idSoundVoice;

	int					lastResetTime;

	int					outputChannels;
	int					channelMask;

	idDebugGraph *		vuMeterRMS;
	idDebugGraph *		vuMeterPeak;
	int					vuMeterPeakTimes[ 8 ];
};

#endif /* !__SOUNDHARDWARE_H__ */