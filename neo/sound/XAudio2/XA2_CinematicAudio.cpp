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
#include "precompiled.h"
#pragma hdrstop

#include "../snd_local.h"

#undef nullptr

#include "libbinkdec/include/BinkDecoder.h"

idCVar s_playCinematicAudio( "s_playCinematicAudio", "1", CVAR_BOOL, "Play audio if available in cinematic video files" );

/*
========================
idVoiceCallback
========================
*/
class idVoiceCallback : public IXAudio2VoiceCallback {
public:
	STDMETHOD_(void, OnBufferEnd)( void* pContext ) {
		Mem_Free( pContext );
		pContext = NULL;
	}

	//Unused methods are stubs
	STDMETHOD_(void, OnBufferStart)( void * pContext ) {}
	STDMETHOD_(void, OnLoopEnd)( void * ) {}
	STDMETHOD_(void, OnStreamEnd)() {}
	STDMETHOD_(void, OnVoiceError)( void *, HRESULT hr ) {}
	STDMETHOD_(void, OnVoiceProcessingPassEnd)() {}
	STDMETHOD_(void, OnVoiceProcessingPassStart)( UINT32 BytesRequired ) {}

};

idVoiceCallback voiceCallback;

/*
==============
idCinematicAudio_XAudio2::InitAudio
==============
*/
void idCinematicAudio_XAudio2::InitAudio( void *audioContext ) {
	AudioInfo *binkInfo = (AudioInfo *)audioContext;
	int format_byte = 2;
	bool use_ext = false;
	voiceFormatcine.nChannels = binkInfo->nChannels; //fixed
	voiceFormatcine.nSamplesPerSec = binkInfo->sampleRate; //fixed

	WAVEFORMATEXTENSIBLE exvoice = { 0 };
	voiceFormatcine.wFormatTag = WAVE_FORMAT_EXTENSIBLE; //Use extensible wave format in order to handle properly the audio
	voiceFormatcine.wBitsPerSample = format_byte * 8; //fixed
	voiceFormatcine.nBlockAlign = format_byte * voiceFormatcine.nChannels; //fixed
	voiceFormatcine.nAvgBytesPerSec = voiceFormatcine.nSamplesPerSec * voiceFormatcine.nBlockAlign; //fixed
	voiceFormatcine.cbSize = 22; //fixed
	exvoice.Format = voiceFormatcine;
	switch ( voiceFormatcine.nChannels ) {
		case 1:
			exvoice.dwChannelMask = SPEAKER_MONO;
			break;
		case 2:
			exvoice.dwChannelMask = SPEAKER_STEREO;
			break;
		case 4:
			exvoice.dwChannelMask = SPEAKER_QUAD;
			break;
		case 5:
			exvoice.dwChannelMask = SPEAKER_5POINT1_SURROUND;
			break;
		case 7:
			exvoice.dwChannelMask = SPEAKER_7POINT1_SURROUND;
			break;
		default:
			exvoice.dwChannelMask = SPEAKER_MONO;
			break;
	}
	exvoice.Samples.wReserved = 0;
	exvoice.Samples.wValidBitsPerSample = voiceFormatcine.wBitsPerSample;
	exvoice.Samples.wSamplesPerBlock = voiceFormatcine.wBitsPerSample;
	exvoice.SubFormat = use_ext ? KSDATAFORMAT_SUBTYPE_IEEE_FLOAT : KSDATAFORMAT_SUBTYPE_PCM;

	// Use the XAudio2 that the game has initialized instead of making our own
	// SRS - Hook up the voice callback interface to get notice when audio buffers can be freed
	if ( soundSystemLocal.GetAudioAPI() ) {
		((IXAudio2 *)soundSystemLocal.GetAudioAPI())->CreateSourceVoice( &pMusicSourceVoice1, (WAVEFORMATEX *)&exvoice, XAUDIO2_VOICE_USEFILTER, XAUDIO2_DEFAULT_FREQ_RATIO, &voiceCallback );
	}
}

/*
==============
idCinematicAudio_XAudio2::PlayAudio
==============
*/
void idCinematicAudio_XAudio2::PlayAudio( uint8 *data, int size ) {
	//Store the data to XAudio2 buffer
	Packet.Flags = XAUDIO2_END_OF_STREAM;
	Packet.AudioBytes = size;
	Packet.pAudioData = (BYTE *)data;
	Packet.PlayBegin = 0;
	Packet.PlayLength = 0;
	Packet.LoopBegin = 0;
	Packet.LoopLength = 0;
	Packet.LoopCount = 0;
	Packet.pContext = (BYTE *)data;
	HRESULT hr;
	if ( FAILED( hr = pMusicSourceVoice1->SubmitSourceBuffer( &Packet ) ) ) {
		Mem_Free( data );
		data = NULL;
		idLib::Warning( "Failed to submit audio buffer to XAudio2 source voice for cinematic audio: 0x%08X", hr );
	}

	// Play the source voice
	if ( FAILED( hr = pMusicSourceVoice1->Start( 0 ) ) ) {
		idLib::Warning( "Failed to start XAudio2 source voice for cinematic audio: 0x%08X", hr );
	}
}

/*
==============
idCinematicAudio_XAudio2::ResetAudio
==============
*/
void idCinematicAudio_XAudio2::ResetAudio() {
	if ( pMusicSourceVoice1 ) {
		pMusicSourceVoice1->Stop();
		pMusicSourceVoice1->FlushSourceBuffers();
	}
}

/*
==============
idCinematicAudio_XAudio2::ShutdownAudio
==============
*/
void idCinematicAudio_XAudio2::ShutdownAudio() {
	if ( pMusicSourceVoice1 ) {
		pMusicSourceVoice1->Stop();
		pMusicSourceVoice1->FlushSourceBuffers();
		pMusicSourceVoice1->DestroyVoice();
		pMusicSourceVoice1 = NULL;
	}
}
