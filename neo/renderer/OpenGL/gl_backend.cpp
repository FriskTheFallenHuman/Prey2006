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

#include "../tr_local.h"
#include "../../framework/Common_local.h"

idCVar r_drawFlickerBox( "r_drawFlickerBox", "0", CVAR_RENDERER | CVAR_BOOL, "visual test for dropping frames" );
idCVar r_showSwapBuffers( "r_showSwapBuffers", "0", CVAR_BOOL, "Show timings from GL_BlockingSwapBuffers" );
idCVar r_syncEveryFrame( "r_syncEveryFrame", "1", CVAR_BOOL, "Don't let the GPU buffer execution past swapbuffers" );

static int		swapIndex;		// 0 or 1 into renderSync
static GLsync	renderSync[2];

void GLimp_SwapBuffers();
void RB_SetMVP( const idRenderMatrix & mvp );

/*
============================================================================

RENDER BACK END THREAD FUNCTIONS

============================================================================
*/

/*
=============
RB_DrawFlickerBox
=============
*/
static void RB_DrawFlickerBox() {
	if ( !r_drawFlickerBox.GetBool() ) {
		return;
	}
	if ( tr.frameCount & 1 ) {
		qglClearColor( 1, 0, 0, 1 );
	} else {
		qglClearColor( 0, 1, 0, 1 );
	}
	qglScissor( 0, 0, 256, 256 );
	qglClear( GL_COLOR_BUFFER_BIT );
}

/*
=============
RB_SetBuffer
=============
*/
static void	RB_SetBuffer( const void *data ) {
	// see which draw buffer we want to render the frame to

	const setBufferCommand_t * cmd = (const setBufferCommand_t *)data;

	RENDERLOG_PRINTF( "---------- RB_SetBuffer ---------- to buffer # %d\n", cmd->buffer );

	GL_Scissor( 0, 0, tr.GetWidth(), tr.GetHeight() );

	// clear screen for debugging
	// automatically enable this with several other debug tools
	// that might leave unrendered portions of the screen
	if ( r_clear.GetFloat() || idStr::Length( r_clear.GetString() ) != 1 || r_singleArea.GetBool() || r_showOverDraw.GetBool() ) {
		float c[3];
		if ( sscanf( r_clear.GetString(), "%f %f %f", &c[0], &c[1], &c[2] ) == 3 ) {
			GL_Clear( true, false, false, 0, c[0], c[1], c[2], 1.0f );
		} else if ( r_clear.GetInteger() == 2 ) {
			GL_Clear( true, false, false, 0, 0.0f, 0.0f,  0.0f, 1.0f );
		} else if ( r_showOverDraw.GetBool() ) {
			GL_Clear( true, false, false, 0, 1.0f, 1.0f, 1.0f, 1.0f );
		} else {
			GL_Clear( true, false, false, 0, 0.4f, 0.0f, 0.25f, 1.0f );
		}
	}
}

/*
=============
GL_BlockingSwapBuffers

We want to exit this with the GPU idle, right at vsync
=============
*/
const void GL_BlockingSwapBuffers() {
    RENDERLOG_PRINTF( "***************** GL_BlockingSwapBuffers *****************\n\n\n" );

	const int beforeFinish = Sys_Milliseconds();

	if ( !glConfig.syncAvailable ) {
		glFinish();
	}

	const int beforeSwap = Sys_Milliseconds();
	if ( r_showSwapBuffers.GetBool() && beforeSwap - beforeFinish > 1 ) {
		common->Printf( "%i msec to glFinish\n", beforeSwap - beforeFinish );
	}

	GLimp_SwapBuffers();

	const int beforeFence = Sys_Milliseconds();
	if ( r_showSwapBuffers.GetBool() && beforeFence - beforeSwap > 1 ) {
		common->Printf( "%i msec to swapBuffers\n", beforeFence - beforeSwap );
	}

	if ( glConfig.syncAvailable ) {
		swapIndex ^= 1;

		if ( qglIsSync( renderSync[swapIndex] ) ) {
			qglDeleteSync( renderSync[swapIndex] );
		}
		// draw something tiny to ensure the sync is after the swap
		const int start = Sys_Milliseconds();
		qglScissor( 0, 0, 1, 1 );
		qglEnable( GL_SCISSOR_TEST );
		qglClear( GL_COLOR_BUFFER_BIT );
		renderSync[swapIndex] = qglFenceSync( GL_SYNC_GPU_COMMANDS_COMPLETE, 0 );
		const int end = Sys_Milliseconds();
		if ( r_showSwapBuffers.GetBool() && end - start > 1 ) {
			common->Printf( "%i msec to start fence\n", end - start );
		}

		GLsync	syncToWaitOn;
		if ( r_syncEveryFrame.GetBool() ) {
			syncToWaitOn = renderSync[swapIndex];
		} else {
			syncToWaitOn = renderSync[!swapIndex];
		}

		if ( qglIsSync( syncToWaitOn ) ) {
			for ( GLenum r = GL_TIMEOUT_EXPIRED; r == GL_TIMEOUT_EXPIRED; ) {
				r = qglClientWaitSync( syncToWaitOn, GL_SYNC_FLUSH_COMMANDS_BIT, 1000 * 1000 );
			}
		}
	}

	const int afterFence = Sys_Milliseconds();
	if ( r_showSwapBuffers.GetBool() && afterFence - beforeFence > 1 ) {
		common->Printf( "%i msec to wait on fence\n", afterFence - beforeFence );
	}

	const int64 exitBlockTime = Sys_Microseconds();

	static int64 prevBlockTime;
	if ( r_showSwapBuffers.GetBool() && prevBlockTime ) {
		const int delta = (int) ( exitBlockTime - prevBlockTime );
		common->Printf( "blockToBlock: %i\n", delta );
	}
	prevBlockTime = exitBlockTime;
}

/*
====================
RB_ExecuteBackEndCommands

This function will be called syncronously if running without
smp extensions, or asyncronously by another thread.
====================
*/
void RB_ExecuteBackEndCommands( const emptyCommand_t *cmds ) {
	// r_debugRenderToTexture
	int c_draw3d = 0;
	int c_draw2d = 0;
	int c_setBuffers = 0;
	int c_copyRenders = 0;

	resolutionScale.SetCurrentGPUFrameTime( commonLocal.GetRendererGPUMicroseconds() );

	renderLog.StartFrame();

	if ( cmds->commandId == RC_NOP && !cmds->next ) {
		return;
	}

	uint64 backEndStartTime = Sys_Microseconds();

	// needed for editor rendering
	GL_SetDefaultState();

	for ( ; cmds != NULL; cmds = (const emptyCommand_t *)cmds->next ) {
		switch ( cmds->commandId ) {
		case RC_NOP:
			break;
		case RC_DRAW_VIEW_3D:
		case RC_DRAW_VIEW_GUI:
			RB_DrawView( cmds );
			if ( ((const drawSurfsCommand_t *)cmds)->viewDef->viewEntitys ) {
				c_draw3d++;
			} else {
				c_draw2d++;
			}
			break;
		case RC_SET_BUFFER:
			c_setBuffers++;
			break;
		case RC_COPY_RENDER:
			RB_CopyRender( cmds );
			c_copyRenders++;
			break;
		case RC_POST_PROCESS:
			RB_PostProcess( cmds );
			break;
		default:
			common->Error( "RB_ExecuteBackEndCommands: bad commandId" );
			break;
		}
	}

	RB_DrawFlickerBox();

	// Fix for the steam overlay not showing up while in game without Shell/Debug/Console/Menu also rendering
	qglColorMask( 1, 1, 1, 1 );

	qglFlush();

	// stop rendering on this thread
	uint64 backEndFinishTime = Sys_Microseconds();
	backEnd.pc.totalMicroSec = backEndFinishTime - backEndStartTime;

	if ( r_debugRenderToTexture.GetInteger() == 1 ) {
		common->Printf( "3d: %i, 2d: %i, SetBuf: %i, CpyRenders: %i, CpyFrameBuf: %i\n", c_draw3d, c_draw2d, c_setBuffers, c_copyRenders, backEnd.pc.c_copyFrameBuffer );
		backEnd.pc.c_copyFrameBuffer = 0;
	}
	renderLog.EndFrame();
}

/*
=============
RB_SetGL2D

This is not used by the normal game paths, just by some tools
=============
*/
void RB_SetGL2D() {
	// set 2D virtual screen size
	qglViewport( 0, 0, glConfig.nativeScreenWidth, glConfig.nativeScreenHeight );
	if ( r_useScissor.GetBool() ) {
		qglScissor( 0, 0, glConfig.nativeScreenWidth, glConfig.nativeScreenHeight );
	}
	qglMatrixMode( GL_PROJECTION );
	qglLoadIdentity();
	qglOrtho( 0, 640, 480, 0, 0, 1 );		// always assume 640x480 virtual coordinates
	qglMatrixMode( GL_MODELVIEW );
	qglLoadIdentity();

	GL_State( GLS_DEPTHFUNC_ALWAYS |
			  GLS_SRCBLEND_SRC_ALPHA |
			  GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );

	GL_Cull( CT_TWO_SIDED );

	qglDisable( GL_DEPTH_TEST );
	qglDisable( GL_STENCIL_TEST );
}