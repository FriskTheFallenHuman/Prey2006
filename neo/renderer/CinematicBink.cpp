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

#include "tr_local.h"

#if defined( _MSC_VER )
	#include <xaudio2.h>
	#include "../sound/XAudio2/XA2_CinematicAudio.h"
#endif

extern idCVar s_noSound;
extern idCVar s_playCinematicAudio;

const int DEFAULT_CIN_WIDTH = 512;
const int DEFAULT_CIN_HEIGHT = 512;

/*
==============
idCinematicBink::idCinematicBink
==============
*/
idCinematicBink::idCinematicBink() {
	binkHandle.isValid = false;
	binkHandle.instanceIndex = -1; // whatever this is, it now has a deterministic value

	hasFrame = false;
	framePos = -1;
	numFrames = 0;

	audioTracks = 0;
	trackIndex = -1;

	binkInfo = {};

	imgY = globalImages->AllocStandaloneImage( "_cinematicY" );
	imgCr = globalImages->AllocStandaloneImage( "_cinematicCr" );
	imgCb = globalImages->AllocStandaloneImage( "_cinematicCb" );

	idImageOpts opts;
	opts.format = FMT_LUM8;
	opts.colorFormat = CFM_DEFAULT;
	opts.width = 32;
	opts.height = 32;
	opts.numLevels = 1;
	if ( imgY != NULL ) {
		imgY->AllocImage( opts, TF_LINEAR, TR_REPEAT );
	}
	if ( imgCr != NULL ) {
		imgCr->AllocImage( opts, TF_LINEAR, TR_REPEAT );
	}
	if ( imgCb != NULL ) {
		imgCb->AllocImage( opts, TF_LINEAR, TR_REPEAT );
	}
}

/*
==============
idCinematicBink::~idCinematicBink
==============
*/
idCinematicBink::~idCinematicBink() {
	Close();

	delete imgY;
	imgY = NULL;
	delete imgCr;
	imgCr = NULL;
	delete imgCb;
	imgCb = NULL;

	if ( cinematicAudio ) {
		cinematicAudio->ShutdownAudio();
		delete cinematicAudio;
		cinematicAudio = NULL;
	}
}

/*
==============
idCinematicBink::InitFromFile
==============
*/
bool idCinematicBink::InitFromFile( const char* qpath, bool amilooping ) {
	animationLength = 0;
	return InitBinkFile( qpath, amilooping );
}

/*
==============
idCinematicBink::InitBinkFile
==============
*/
bool idCinematicBink::InitBinkFile( const char* qpath, bool amilooping ) {
	looping = amilooping;
	startTime = 0;
	CIN_HEIGHT = DEFAULT_CIN_HEIGHT;
	CIN_WIDTH  =  DEFAULT_CIN_WIDTH;

	idStr fullpath;
	idFile *testFile = fileSystem->OpenFileRead( qpath );
	if(  testFile ) {
		fullpath = testFile->GetFullPath();
		fileSystem->CloseFile( testFile );
	} else if( idStr::Cmpn( qpath, "sound/vo", 8 ) == 0 ) {
		idStr newPath( qpath );
		newPath.Replace( "sound/vo", "sound/VO" );

		testFile = fileSystem->OpenFileRead( newPath );
		if ( testFile ) {
			fullpath = testFile->GetFullPath();
			fileSystem->CloseFile( testFile );
		} else {
			common->Warning( "idCinematicBink: Cannot open Bink video file: '%s', %d\n", qpath, looping );
			return false;
		}
	}

	binkHandle = Bink_Open( fullpath );

	if ( !binkHandle.isValid ) {
		common->Warning( "idCinematicBink: Cannot open Bink video file: '%s', %d\n", qpath, looping );
		return false;
	}

	{
		uint32_t w = 0, h = 0;
		Bink_GetFrameSize( binkHandle, w, h );
		CIN_WIDTH = w;
		CIN_HEIGHT = h;
	}

	audioTracks = Bink_GetNumAudioTracks( binkHandle );
	if ( audioTracks > 0 && s_playCinematicAudio.GetBool() ) {
		trackIndex = 0;
		binkInfo = Bink_GetAudioTrackDetails( binkHandle, trackIndex );

		cinematicAudio = new( TAG_AUDIO ) idCinematicAudio_XAudio2;
		cinematicAudio->InitAudio( &binkInfo );
	}

	frameRate = Bink_GetFrameRate( binkHandle );
	numFrames = Bink_GetNumFrames( binkHandle );
	float durationSec = numFrames/frameRate;
	animationLength = durationSec * 1000;
	common->Printf( S_COLOR_GRAY "  ...bink file: " S_COLOR_WHITE "'%s'\n", qpath );

	memset( yuvBuffer, 0, sizeof( yuvBuffer ) );

	status = FMV_PLAY;
	hasFrame = false;
	framePos = -1;
	ImageForTime(0);
	status = (looping) ? FMV_PLAY : FMV_IDLE;

	return true;
}

/*
==============
idCinematicBink::ExportToTGA
==============
*/
void idCinematicBink::ExportToTGA( bool skipExisting ) {
}

/*
==============
idCinematicBink::GetFrameRate
==============
*/
float idCinematicBink::GetFrameRate() const {
	return frameRate;
}


/*
==============
idCinematicBink::Close
==============
*/
void idCinematicBink::Close() {
	if ( binkHandle.isValid ) {
		memset( yuvBuffer, 0 , sizeof( yuvBuffer ) );
		Bink_Close( binkHandle );
	}
	status = FMV_EOF;
}

/*
==============
idCinematicBink::AnimationLength
==============
*/
int idCinematicBink::AnimationLength() {
	return animationLength;
}

/*
==============
idCinematicBink::IsPlaying
==============
*/
bool idCinematicBink::IsPlaying() const {
	return ( status == FMV_PLAY );
}

/*
==============
idCinematicBink::GetStartTime
==============
*/
int idCinematicBink::GetStartTime() {
	return startTime;
}

/*
==============
idCinematicBink::ResetTime
==============
*/
void idCinematicBink::ResetTime( int time ) {
	startTime = time; //originally this was: ( backEnd.viewDef ) ? 1000 * backEnd.viewDef->floatTime : -1;
	status = FMV_PLAY;
}

/*
==============
idCinematicBink::ImageForTime
==============
*/
cinData_t idCinematicBink::ImageForTime( int milliseconds ) {
	cinData_t	cinData;
	int16 *	audioBuffer = NULL;
	uint32	num_bytes = 0;

	if ( milliseconds <= 0 ) {
		milliseconds = Sys_Milliseconds();
	}

	memset( &cinData, 0, sizeof( cinData ) );

	if ( r_skipVideo.GetBool() || status == FMV_EOF || status == FMV_IDLE ) {
		return cinData;
	}

	if ( !binkHandle.isValid ) {
		return cinData;
	}

	if ( ( !hasFrame ) || startTime == -1 ) {
		if ( startTime == -1 ) {
			BinkDecReset();
		}
		startTime = milliseconds;
	}

	int desiredFrame = ( ( milliseconds - startTime ) * frameRate ) / 1000.0f;
	if ( desiredFrame < 0 ) {
		desiredFrame = 0;
	}

	if ( desiredFrame < framePos ) {
		BinkDecReset();
		hasFrame = false;
		status = FMV_PLAY;
	}

	if ( hasFrame && desiredFrame == framePos ) {
		cinData.imageWidth = CIN_WIDTH;
		cinData.imageHeight = CIN_HEIGHT;
		cinData.status = status;

		cinData.imageY = imgY;
		cinData.imageCr = imgCr;
		cinData.imageCb = imgCb;
		return cinData;
	}

	if ( desiredFrame >= numFrames ) {
		status = FMV_EOF;
		if ( looping ) {
			desiredFrame = 0;
			BinkDecReset();
			hasFrame = false;
			startTime = milliseconds;
			status = FMV_PLAY;
		} else {
			hasFrame = false;
			status = FMV_IDLE;
			return cinData;
		}
	}

	// Bink_GotoFrame(binkHandle, desiredFrame);
	// apparently Bink_GotoFrame() doesn't work super well, so skip frames
	// (if necessary) by calling Bink_GetNextFrame()
	while ( framePos < desiredFrame ) {
		framePos = Bink_GetNextFrame( binkHandle, yuvBuffer );
	}

	cinData.imageWidth = CIN_WIDTH;
	cinData.imageHeight = CIN_HEIGHT;
	cinData.status = status;

	double invAspRat = double( CIN_HEIGHT ) / double( CIN_WIDTH );

	idImage *imgs[ 3 ] = { imgY, imgCb, imgCr }; // that's the order of the channels in yuvBuffer[]
	for ( int i = 0; i < 3; ++i ) {
		// Note: img->UploadScratch() seems to assume 32bit per pixel data, but this is 8bit/pixel
		//       so uploading is a bit more manual here (compared to ffmpeg or RoQ)
		idImage* img = imgs[i];
		int w = yuvBuffer[i].width;
		int h = yuvBuffer[i].height;
		// some videos, including the logo video and the main menu background,
		// seem to have superfluous rows in at least some of the channels,
		// leading to a black or glitchy bar at the bottom of the video.
		// cut that off by reducing the height to the expected height
		if ( h > CIN_HEIGHT ) {
			h = CIN_HEIGHT;
		} else if( h < CIN_HEIGHT ) {
			// the U and V channels have a lower resolution than the Y channel
			// (or the logical video resolution), so use the aspect ratio to
			// calculate the real height
			int hExp = invAspRat * w + 0.5;
			if ( h > hExp ) {
				h = hExp;
			}
		}

		if ( img->GetUploadWidth() != w || img->GetUploadHeight() != h ) {
			idImageOpts opts = img->GetOpts();
			opts.width = w;
			opts.height = h;
			img->AllocImage( opts, TF_LINEAR, TR_REPEAT );
		}
		img->SubImageUpload( 0, 0, 0, 0, w, h, yuvBuffer[i].data );
	}

	hasFrame = true;
	cinData.imageY = imgY;
	cinData.imageCr = imgCr;
	cinData.imageCb = imgCb;

	if ( cinematicAudio ) {
		audioBuffer = ( int16* )Mem_Alloc( binkInfo.idealBufferSize, TAG_AUDIO );
		num_bytes = Bink_GetAudioData( binkHandle, trackIndex, audioBuffer );

		if ( num_bytes > 0 && !s_noSound.GetBool() ) {
			cinematicAudio->PlayAudio( ( uint8* )audioBuffer, num_bytes );
		} else {
			Mem_Free( audioBuffer );
			audioBuffer = NULL;
		}
	}

	return cinData;
}

/*
==============
idCinematicBink::BinkDecReset
==============
*/
void idCinematicBink::BinkDecReset() {
	framePos = -1;

	if ( cinematicAudio ) {
		cinematicAudio->ResetAudio();
	}

	Bink_GotoFrame( binkHandle, 0 );
	status = FMV_LOOPED;
}