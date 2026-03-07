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

#ifndef __DEVICECONTEXT_H__
#define __DEVICECONTEXT_H__

// device context support for gui stuff
//

#include "Rectangle.h"
#include "../renderer/Font.h"

const int VIRTUAL_WIDTH = 640;
const int VIRTUAL_HEIGHT = 480;
const int BLINK_DIVISOR = 200;

enum eCursorType {
	CURSOR_ARROW,
	CURSOR_HAND,
	CURSOR_HAND_JOY1,
	CURSOR_HAND_JOY2,
	CURSOR_HAND_JOY3,
	CURSOR_HAND_JOY4,
	CURSOR_COUNT
};

enum eAlignType {
	ALIGN_LEFT,
	ALIGN_CENTER,
	ALIGN_RIGHT
};

enum eScrollBarType {
	SCROLLBAR_HBACK,
	SCROLLBAR_VBACK,
	SCROLLBAR_THUMB,
	SCROLLBAR_RIGHT,
	SCROLLBAR_LEFT,
	SCROLLBAR_UP,
	SCROLLBAR_DOWN,
	SCROLLBAR_COUNT
};

class idDeviceContext {
public:
	virtual				~idDeviceContext() {}

	// initialization
	virtual void		Init() = 0;
	virtual void		Shutdown() = 0;
	virtual bool		Initialized() = 0;
	virtual void		Clear() = 0;

	// transform info
	virtual void		GetTransformInfo( idVec3 &origin, idMat3 &mat ) = 0;
	virtual void		SetTransformInfo( const idVec3 &origin, const idMat3 &mat ) = 0;

	// drawing
	virtual void		DrawMaterial( float x, float y, float w, float h, const idMaterial *mat, const idVec4 &color, float scalex = 1.0, float scaley = 1.0 ) = 0;
	virtual void		DrawRect( float x, float y, float width, float height, float size, const idVec4 &color ) = 0;
	virtual void		DrawFilledRect( float x, float y, float width, float height, const idVec4 &color ) = 0;
	virtual int			DrawText( const char *text, float textScale, int textAlign, idVec4 color, idRectangle rectDraw, bool wrap, int cursor = -1, bool calcOnly = false, idList<int> *breaks = NULL, int limit = 0 ) = 0;
	virtual int			DrawText( float x, float y, float scale, idVec4 color, const char *text, float adjust, int limit, int style, int cursor = -1 ) = 0;
	virtual void		DrawMaterialRect( float x, float y, float w, float h, float size, const idMaterial *mat, const idVec4 &color ) = 0;
	virtual void		DrawStretchPic( float x, float y, float w, float h, float s0, float t0, float s1, float t1, const idMaterial *mat ) = 0;
	virtual void		DrawMaterialRotated( float x, float y, float w, float h, const idMaterial *mat, const idVec4 &color, float scalex = 1.0, float scaley = 1.0, float angle = 0.0f ) = 0;
	virtual void		DrawStretchPicRotated( float x, float y, float w, float h, float s0, float t0, float s1, float t1, const idMaterial *mat, float angle = 0.0f ) = 0;
	virtual void		DrawWinding( idWinding &w, const idMaterial *mat ) = 0;

	// text drawing
	virtual int			CharWidth( const char c, float scale ) = 0;
	virtual int			TextWidth( const char *text, float scale, int limit ) = 0;
	virtual int			TextHeight( const char *text, float scale, int limit ) = 0;
	virtual int			MaxCharHeight( float scale ) = 0;
	virtual int			MaxCharWidth( float scale ) = 0;
	virtual idRegion *	GetTextRegion( const char *text, float textScale, idRectangle rectDraw, float xStart, float yStart ) = 0;
	virtual void		SetOverStrike( bool b ) = 0;
	virtual bool		GetOverStrike() = 0;
	virtual void		PaintChar( float x, float y, const scaledGlyphInfo_t &glyphInfo ) = 0;

	// size and offset
	virtual void		SetSize( float width, float height ) = 0;
	virtual void		SetOffset( float x, float y ) = 0;

	// scroll bar images
	virtual const idMaterial *GetScrollBarImage( int index ) = 0;

	// cursor drawing
	virtual void		DrawCursor( float *x, float *y, float size ) = 0;
	virtual void		DrawEditCursor( float x, float y, float scale ) = 0;
	virtual void		SetCursor( int n ) = 0;

	// clipping rects
	virtual bool		ClippedCoords( float *x, float *y, float *w, float *h, float *s1, float *t1, float *s2, float *t2 ) = 0;
	virtual void		PushClipRect( idRectangle r ) = 0;
	virtual void		PopClipRect() = 0;
	virtual void		EnableClipping( bool b ) = 0;

	// fonts
	virtual void		InitFonts() = 0;
	virtual void		SetFont( idFont * font ) = 0;
	virtual void		SetFont( int num ) = 0;
	virtual int			FindFont( const char *name ) = 0;
};

class idDeviceContextLocal : public idDeviceContext {
public:
						idDeviceContextLocal();

	virtual void		Init();
	virtual void		InitFonts();
	virtual void		Shutdown();
	virtual bool		Initialized() { return initialized; }

	virtual void		GetTransformInfo(idVec3& origin, idMat3& mat );

	virtual void		SetTransformInfo(const idVec3 &origin, const idMat3 &mat);
	virtual void		DrawMaterial(float x, float y, float w, float h, const idMaterial *mat, const idVec4 &color, float scalex = 1.0, float scaley = 1.0);
	virtual void		DrawRect(float x, float y, float width, float height, float size, const idVec4 &color);
	virtual void		DrawFilledRect(float x, float y, float width, float height, const idVec4 &color);
	virtual int			DrawText(const char *text, float textScale, int textAlign, idVec4 color, idRectangle rectDraw, bool wrap, int cursor = -1, bool calcOnly = false, idList<int> *breaks = NULL, int limit = 0 );
	virtual void		DrawMaterialRect( float x, float y, float w, float h, float size, const idMaterial *mat, const idVec4 &color);
	virtual void		DrawStretchPic(float x, float y, float w, float h, float s0, float t0, float s1, float t1, const idMaterial *mat);
	virtual void		DrawMaterialRotated(float x, float y, float w, float h, const idMaterial *mat, const idVec4 &color, float scalex = 1.0, float scaley = 1.0, float angle = 0.0f);
	virtual void		DrawStretchPicRotated(float x, float y, float w, float h, float s0, float t0, float s1, float t1, const idMaterial *mat, float angle = 0.0f);
	virtual void		DrawWinding( idWinding & w, const idMaterial * mat );

	virtual int			CharWidth( const char c, float scale );
	virtual int			TextWidth( const char *text, float scale, int limit );
	virtual int			TextHeight( const char *text, float scale, int limit );
	virtual int			MaxCharHeight( float scale );
	virtual int			MaxCharWidth( float scale );

	virtual idRegion *	GetTextRegion(const char *text, float textScale, idRectangle rectDraw, float xStart, float yStart);

	virtual void		SetSize( float width, float height );
	virtual void		SetOffset( float x, float y );

	virtual const idMaterial *	GetScrollBarImage(int index);

	virtual void		DrawCursor(float *x, float *y, float size);
	virtual void		SetCursor(int n);

	// clipping rects
	virtual bool		ClippedCoords(float *x, float *y, float *w, float *h, float *s1, float *t1, float *s2, float *t2);
	virtual void		PushClipRect(idRectangle r);
	virtual void		PopClipRect();
	virtual void		EnableClipping(bool b);

	virtual void		SetFont( idFont * font ) { activeFont = font; }
	virtual void		SetFont( int num ) {}
	virtual int			FindFont( const char * name ) { return 0; }

	virtual void		SetOverStrike( bool b ) { overStrikeMode = b; }

	virtual bool		GetOverStrike() { return overStrikeMode; }

	virtual void		DrawEditCursor( float x, float y, float scale );

	virtual int			DrawText( float x, float y, float scale, idVec4 color, const char *text, float adjust, int limit, int style, int cursor = -1);
	virtual void		PaintChar( float x, float y, const scaledGlyphInfo_t & glyphInfo );
	virtual void		Clear();

	static idVec4 colorPurple;
	static idVec4 colorOrange;
	static idVec4 colorYellow;
	static idVec4 colorGreen;
	static idVec4 colorBlue;
	static idVec4 colorRed;
	static idVec4 colorWhite;
	static idVec4 colorBlack;
	static idVec4 colorNone;

protected:

	const idMaterial *	cursorImages[CURSOR_COUNT];
	const idMaterial *	scrollBarImages[SCROLLBAR_COUNT];
	const idMaterial *	whiteImage;
	idFont *			activeFont;

	float				xScale;
	float				yScale;
	float				xOffset;
	float				yOffset;

	int					cursor;

	idList<idRectangle>	clipRects;

	bool				enableClipping;

	bool				overStrikeMode;

	idMat3				mat;
	bool				matIsIdentity;
	idVec3				origin;
	bool				initialized;
};

class idDeviceContextLegacy : public idDeviceContextLocal {
public:
	virtual void		InitFonts();
	virtual void		Shutdown();
	virtual void		Clear();

	virtual int			DrawText( float x, float y, float scale, idVec4 color, const char *text, float adjust, int limit, int style, int cursor = -1);

	virtual int			CharWidth( const char c, float scale );
	virtual int			TextWidth( const char *text, float scale, int limit );
	virtual int			TextHeight( const char *text, float scale, int limit );
	virtual int			MaxCharHeight( float scale );
	virtual int			MaxCharWidth( float scale );

	virtual void		DrawEditCursor( float x, float y, float scale );

	virtual void		SetFont( int num );
	virtual int			FindFont( const char * name );

protected:
	virtual int			DrawText( const char *text, float textScale, int textAlign, idVec4 color, idRectangle rectDraw, bool wrap, int cursor = -1, bool calcOnly = false, idList<int> *breaks = NULL, int limit = 0 );
	virtual void		PaintChar( float x, float y, float width, float height, float scale, float	s, float	t, float	s2, float t2, const idMaterial* hShader );
	virtual void		SetFontByScale( float scale );

private:
	void				SetupFonts();

	fontInfoEx_t*		activeFont;
	fontInfo_t*			useFont;
	idStr				fontName;

	static idList<fontInfoEx_t> fonts;
	idStr				fontLang;
};

class idDeviceContextOptimized : public idDeviceContextLocal {

	virtual bool		ClippedCoords(float *x, float *y, float *w, float *h, float *s1, float *t1, float *s2, float *t2);
	virtual void		PushClipRect(idRectangle r);
	virtual void		PopClipRect();
	virtual void		EnableClipping(bool b);

	virtual int			DrawText( float x, float y, float scale, idVec4 color, const char *text, float adjust, int limit, int style, int cursor = -1);

	float				clipX1;
	float				clipX2;
	float				clipY1;
	float				clipY2;
};

#endif /* !__DEVICECONTEXT_H__ */
