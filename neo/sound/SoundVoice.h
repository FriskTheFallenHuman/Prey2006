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
#ifndef __SOUNDVOICE_H__
#define __SOUNDVOICE_H__

static const int MAX_QUEUED_BUFFERS = 3;

/*
===============================================================================

	SOUND VOICE

===============================================================================
*/

class idSoundVoice : public idSoundVoice_Base {
public:
				idSoundVoice():
					leadinSample( NULL ),
					loopingSample( NULL ),
					formatTag( 0 ),
					numChannels( 0 ),
					sampleRate( 0 ),
					paused( true ),
					hasVUMeter( false ),
					chains( 1 ) {
				}

	virtual void	Create( const idSoundSample * leadinSample, const idSoundSample * loopingSample, const int _channel ) {}

	// Start playing at a particular point in the buffer.  Does an Update() too
	virtual void	Start( int offsetMS, int ssFlags ) {}

	// Stop playing.
	virtual void	Stop() {}

	// Stop consuming buffers
	virtual void	Pause() {}

	// Start consuming buffers again
	virtual void	UnPause() {}

	// Sends new position/volume/pitch information to the hardware
	virtual bool	Update() { return false; }

	// returns the RMS levels of the most recently processed block of audio, SSF_FLICKER must have been passed to Start
	virtual float	GetAmplitude() { return -1.0f; }

	// returns true if we can re-use this voice
	virtual bool	CompatibleFormat( idSoundSample * s ) { return false; }

	uint32			GetSampleRate() const { return sampleRate; }

protected:
	friend class idSoundhardware;
	friend class idSoundSample;

protected:
	idSoundSample * leadinSample;
	idSoundSample * loopingSample;

	// These are the fields from the sample format that matter to us for voice reuse
	uint16		formatTag;
	uint16		numChannels;

	uint32		sourceVoiceRate;
	uint32		sampleRate;

	bool		hasVUMeter;
	bool		paused;

	int			channel;
	int			chains;
};

#endif /* !__SOUNDVOICE_H__ */
