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

#ifndef __CINEMATICROQ_H__
#define __CINEMATICROQ_H__

/*
===============================================================================

	RoQ cinematics

===============================================================================
*/

class idCinematicRoQ : public idCinematic {
public:
					idCinematicRoQ();
	virtual			~idCinematicRoQ();
	virtual bool	InitFromFile( const char *qpath, bool looping );
	virtual int		AnimationLength();
	virtual cinData_t	ImageForTime( int milliseconds );
	virtual void	Close();
	virtual void	ResetTime( int time );
	virtual int		GetStartTime();
	virtual bool    IsPlaying() const;
	virtual void	ExportToTGA( bool skipExisting = true );
	virtual float	GetFrameRate() const;

	static void		InitCinematic();
	static void		ShutdownCinematic();

private:
	void			RoQ_init();
	void			blitVQQuad32fs( byte **status, unsigned char *data );
	void			RoQShutdown();
	void			RoQInterrupt();

	void			move8_32( byte *src, byte *dst, int spl );
	void			move4_32( byte *src, byte *dst, int spl );
	void			blit8_32( byte *src, byte *dst, int spl );
	void			blit4_32( byte *src, byte *dst, int spl );
	void			blit2_32( byte *src, byte *dst, int spl );

	unsigned short	yuv_to_rgb( int y, int u, int v );
	unsigned int	yuv_to_rgb24( int y, int u, int v );

	void			decodeCodeBook( byte *input, unsigned short roq_flags );
	void			recurseQuad( int startX, int startY, int quadSize, int xOff, int yOff );
	void			setupQuad( int xOff, int yOff );
	void			readQuadInfo( byte *qData );
	void			RoQPrepMcomp( int xoff, int yoff );
	void			RoQReset();

private:
	idImage *				img;

	size_t					mcomp[256];
	byte **					qStatus[2];
	idStr					fileName;
	int						CIN_WIDTH, CIN_HEIGHT;
	idFile *				iFile;
	cinStatus_t				status;
	int						tfps;
	int						RoQPlayed;
	int						ROQSize;
	unsigned int			RoQFrameSize;
	int						onQuad;
	int						numQuads;
	int						samplesPerLine;
	unsigned int			roq_id;
	int						screenDelta;
	byte *					buf;
	int						samplesPerPixel;				// defaults to 2
	unsigned int			xsize, ysize, maxsize, minsize;
	int						normalBuffer0;
	int						roq_flags;
	int						roqF0;
	int						roqF1;
	int						t[2];
	int						roqFPS;
	int						drawX, drawY;

	int						animationLength;
	int						startTime;
	float					frameRate;

	byte *					image;

	bool					looping;
	bool					dirty;
	bool					half;
	bool					smootheddouble;
	bool					inMemory;
};

#endif /* !__CINEMATICROQ_H__ */
