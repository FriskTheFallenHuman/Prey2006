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

#include "DeviceContext.h"

idCVar gui_smallFontLimit( "gui_smallFontLimit", "0.0", CVAR_GUI | CVAR_ARCHIVE, "" );
idCVar gui_mediumFontLimit( "gui_mediumFontLimit", "0.0", CVAR_GUI | CVAR_ARCHIVE, "" );

idList<fontInfoEx_t> idDeviceContextLegacy::fonts;

int idDeviceContextLegacy::FindFont( const char *name ) {
	int c = fonts.Num();
	for ( int i = 0; i < c; i++ ) {
		if ( idStr::Icmp( name, fonts[i].name ) == 0 ) {
			return i;
		}
	}

	// If the font was not found, try to register it
	idStr fileName = name;
	fileName.Replace( "fonts", va( "fonts/%s", fontLang.c_str() ) );

	fontInfoEx_t fontInfo;
	int index = fonts.Append( fontInfo );
	if ( renderSystem->RegisterFont( fileName, fonts[index] ) ) {
		idStr::Copynz( fonts[index].name, name, sizeof( fonts[index].name ) );
		return index;
	} else {
		common->Printf( "Could not register font %s [%s]\n", name, fileName.c_str() );
		return -1;
	}
}

void idDeviceContextLegacy::SetupFonts() {
	fonts.SetGranularity( 1 );

	fontLang = cvarSystem->GetCVarString( "sys_lang" );

	// western european languages can use the english font
	if ( fontLang == "french" || fontLang == "german" || fontLang == "spanish" || fontLang == "italian" ) {
		fontLang = "english";
	}

	// Default font has to be added first
	FindFont( "fonts" );
}

void idDeviceContextLegacy::SetFont( int num ) {
	if ( num >= 0 && num < fonts.Num() ) {
		activeFont = &fonts[num];
	} else {
		activeFont = &fonts[0];
	}
}

void idDeviceContextLegacy::InitFonts() {
	SetupFonts();
	activeFont = &fonts[0];
}

void idDeviceContextLegacy::Shutdown() {
	fontName.Clear();
	fonts.Clear();

	idDeviceContextLocal::Shutdown();
}

void idDeviceContextLegacy::Clear() {
	useFont = NULL;
	activeFont = NULL;

	idDeviceContextLocal::Clear();
}

void idDeviceContextLegacy::PaintChar( float x, float y, float width, float height, float scale, float s, float t, float s2, float t2, const idMaterial *hShader ) {
	float	w, h;
	w = width * scale;
	h = height * scale;

	if ( ClippedCoords( &x, &y, &w, &h, &s, &t, &s2, &t2 ) ) {
		return;
	}

	//AdjustCoords( &x, &y, &w, &h );
	DrawStretchPic( x, y, w, h, s, t, s2, t2, hShader );
}


void idDeviceContextLegacy::SetFontByScale( float scale ) {
	if (scale <= gui_smallFontLimit.GetFloat()) {
		useFont = &activeFont->fontInfoSmall;
		activeFont->maxHeight = activeFont->maxHeightSmall;
		activeFont->maxWidth = activeFont->maxWidthSmall;
	} else if (scale <= gui_mediumFontLimit.GetFloat()) {
		useFont = &activeFont->fontInfoMedium;
		activeFont->maxHeight = activeFont->maxHeightMedium;
		activeFont->maxWidth = activeFont->maxWidthMedium;
	} else {
		useFont = &activeFont->fontInfoLarge;
		activeFont->maxHeight = activeFont->maxHeightLarge;
		activeFont->maxWidth = activeFont->maxWidthLarge;
	}
}

int idDeviceContextLegacy::DrawText( float x, float y, float scale, idVec4 color, const char* text, float adjust, int limit, int style, int cursor ) {
	int			len, count;
	idVec4		newColor;
	const glyphInfo_t *glyph;
	float		useScale;
	SetFontByScale(scale);
	useScale = scale * useFont->glyphScale;
	count = 0;
	if ( text && color.w != 0.0f ) {
		const unsigned char	*s = (const unsigned char*)text;
		renderSystem->SetColor(color);
		memcpy(&newColor[0], &color[0], sizeof(idVec4));
		len = strlen(text);
		if (limit > 0 && len > limit) {
			len = limit;
		}

		while (s && *s && count < len) {
			if ( *s < GLYPH_START || *s > GLYPH_END ) {
				s++;
				continue;
			}
			glyph = &useFont->glyphs[*s];

			//
			// int yadj = Assets.textFont.glyphs[text[i]].bottom +
			// Assets.textFont.glyphs[text[i]].top; float yadj = scale *
			// (Assets.textFont.glyphs[text[i]].imageHeight -
			// Assets.textFont.glyphs[text[i]].height);
			//
			if ( idStr::IsColor((const char*)s) ) {
				if ( *(s+1) == C_COLOR_DEFAULT ) {
					newColor = color;
				} else {
					newColor = idStr::ColorForIndex( *(s+1) );
					newColor[3] = color[3];
				}
				if (cursor == count || cursor == count+1) {
					float partialSkip = ((glyph->xSkip * useScale) + adjust) / 5.0f;
					if ( cursor == count ) {
						partialSkip *= 2.0f;
					} else {
						renderSystem->SetColor(newColor);
					}
					DrawEditCursor(x - partialSkip, y, scale);
				}
				renderSystem->SetColor(newColor);
				s += 2;
				count += 2;
				continue;
			} else {
				float yadj = useScale * glyph->top;
				PaintChar(x,y - yadj,glyph->imageWidth,glyph->imageHeight,useScale,glyph->s,glyph->t,glyph->s2,glyph->t2,glyph->glyph);

				if (cursor == count) {
					DrawEditCursor(x, y, scale);
				}
				x += (glyph->xSkip * useScale) + adjust;
				s++;
				count++;
			}
		}
		if (cursor == len) {
			DrawEditCursor(x, y, scale);
		}
	}
	return count;
}

int idDeviceContextLegacy::CharWidth( const char c, float scale ) {
	glyphInfo_t *glyph;
	float		useScale;
	SetFontByScale(scale);
	fontInfo_t	*font = useFont;
	useScale = scale * font->glyphScale;
	glyph = &font->glyphs[(const unsigned char)c];
	return idMath::Ftoi( glyph->xSkip * useScale );
}

int idDeviceContextLegacy::TextWidth( const char * text, float scale, int limit ) {
	int i, width;

	SetFontByScale( scale );
	const glyphInfo_t *glyphs = useFont->glyphs;

	if ( text == NULL ) {
		return 0;
	}

	width = 0;
	if ( limit > 0 ) {
		for ( i = 0; text[i] != '\0' && i < limit; i++ ) {
			if ( idStr::IsColor( text + i ) ) {
				i++;
			} else {
				width += glyphs[((const unsigned char *)text)[i]].xSkip;
			}
		}
	} else {
		for ( i = 0; text[i] != '\0'; i++ ) {
			if ( idStr::IsColor( text + i ) ) {
				i++;
			} else {
				width += glyphs[((const unsigned char *)text)[i]].xSkip;
			}
		}
	}
	return idMath::Ftoi( scale * useFont->glyphScale * width );
}

int idDeviceContextLegacy::TextHeight( const char *text, float scale, int limit ) {
	int			len, count;
	float		max;
	glyphInfo_t *glyph;
	float		useScale;
	const char	*s = text;
	SetFontByScale(scale);
	fontInfo_t	*font = useFont;

	useScale = scale * font->glyphScale;
	max = 0;
	if (text) {
		len = strlen(text);
		if (limit > 0 && len > limit) {
			len = limit;
		}

		count = 0;
		while (s && *s && count < len) {
			if ( idStr::IsColor(s) ) {
				s += 2;
				continue;
			}
			else {
				glyph = &font->glyphs[*(const unsigned char*)s];
				if (max < glyph->height) {
					max = glyph->height;
				}

				s++;
				count++;
			}
		}
	}

	return idMath::Ftoi( max * useScale );
}

int idDeviceContextLegacy::MaxCharWidth( float scale ) {
	SetFontByScale( scale );
	float useScale = scale * useFont->glyphScale;
	return idMath::Ftoi( activeFont->maxWidth * useScale );
}

int idDeviceContextLegacy::MaxCharHeight( float scale ) {
	SetFontByScale( scale );
	float useScale = scale * useFont->glyphScale;
	return idMath::Ftoi( activeFont->maxHeight * useScale );
}

void idDeviceContextLegacy::DrawEditCursor( float x, float y, float scale ) {
	if( (int)( idLib::frameNumber >> 4 ) & 1 ) {
		return;
	}

	SetFontByScale(scale);
	float useScale = scale * useFont->glyphScale;
	const glyphInfo_t *glyph2 = &useFont->glyphs[overStrikeMode ? int('_') : int('|')];
	float	yadj = useScale * glyph2->top;
	PaintChar(x, y - yadj,glyph2->imageWidth,glyph2->imageHeight,useScale,glyph2->s,glyph2->t,glyph2->s2,glyph2->t2,glyph2->glyph);
}

int idDeviceContextLegacy::DrawText( const char *text, float textScale, int textAlign, idVec4 color, idRectangle rectDraw, bool wrap, int cursor, bool calcOnly, idList<int>* breaks, int limit ) {
	const char	*p, *textPtr, *newLinePtr;
	char		buff[1024];
	int			len, newLine, newLineWidth, count;
	float		y;
	float		textWidth;

	float		charSkip = MaxCharWidth( textScale ) + 1;
	float		lineSkip = MaxCharHeight( textScale );

	float		cursorSkip = ( cursor >= 0 ? charSkip : 0 );

	bool		lineBreak, wordBreak;

	SetFontByScale( textScale );

	textWidth = 0;
	newLinePtr = NULL;

	if (!calcOnly && !(text && *text)) {
		if (cursor == 0) {
			renderSystem->SetColor(color);
			DrawEditCursor(rectDraw.x, lineSkip + rectDraw.y, textScale);
		}
		return idMath::Ftoi( rectDraw.w / charSkip );
	}

	textPtr = text;

	y = lineSkip + rectDraw.y;
	len = 0;
	buff[0] = '\0';
	newLine = 0;
	newLineWidth = 0;
	p = textPtr;

	if ( breaks ) {
		breaks->Append(0);
	}
	count = 0;
	textWidth = 0;
	lineBreak = false;
	wordBreak = false;
	while (p) {

		if ( *p == '\n' || *p == '\r' || *p == '\0' ) {
			lineBreak = true;
			if ((*p == '\n' && *(p + 1) == '\r') || (*p == '\r' && *(p + 1) == '\n')) {
				p++;
			}
		}

		int nextCharWidth = ( idStr::CharIsPrintable(*p) ? CharWidth( *p, textScale ) : cursorSkip );
		// FIXME: this is a temp hack until the guis can be fixed not not overflow the bounding rectangles
		//		  the side-effect is that list boxes and edit boxes will draw over their scroll bars
		//	The following line and the !linebreak in the if statement below should be removed
		nextCharWidth = 0;

		if ( !lineBreak && ( textWidth + nextCharWidth ) > rectDraw.w ) {
			// The next character will cause us to overflow, if we haven't yet found a suitable
			// break spot, set it to be this character
			if ( len > 0 && newLine == 0 ) {
				newLine = len;
				newLinePtr = p;
				newLineWidth = textWidth;
			}
			wordBreak = true;
		} else if ( lineBreak || ( wrap && (*p == ' ' || *p == '\t') ) ) {
			// The next character is in view, so if we are a break character, store our position
			newLine = len;
			newLinePtr = p + 1;
			newLineWidth = textWidth;
		}

		if ( lineBreak || wordBreak ) {
			float x = rectDraw.x;

			if (textAlign == ALIGN_RIGHT) {
				x = rectDraw.x + rectDraw.w - newLineWidth;
			} else if (textAlign == ALIGN_CENTER) {
				x = rectDraw.x + (rectDraw.w - newLineWidth) / 2;
			}

			if ( wrap || newLine > 0 ) {
				buff[newLine] = '\0';

				// This is a special case to handle breaking in the middle of a word.
				// if we didn't do this, the cursor would appear on the end of this line
				// and the beginning of the next.
				if ( wordBreak && cursor >= newLine && newLine == len ) {
					cursor++;
				}
			}

			if (!calcOnly) {
				count += DrawText(x, y, textScale, color, buff, 0, 0, 0, cursor);
			}

			if ( cursor < newLine ) {
				cursor = -1;
			} else if ( cursor >= 0 ) {
				cursor -= ( newLine + 1 );
			}

			if ( !wrap ) {
				return newLine;
			}

			if ( ( limit && count > limit ) || *p == '\0' ) {
				break;
			}

			y += lineSkip + 5;

			if ( !calcOnly && y > rectDraw.Bottom() ) {
				break;
			}

			p = newLinePtr;

			if (breaks) {
				breaks->Append(p - text);
			}

			len = 0;
			newLine = 0;
			newLineWidth = 0;
			textWidth = 0;
			lineBreak = false;
			wordBreak = false;
			continue;
		}

		buff[len++] = *p++;
		buff[len] = '\0';
		// update the width
		if ( *( buff + len - 1 ) != C_COLOR_ESCAPE && (len <= 1 || *( buff + len - 2 ) != C_COLOR_ESCAPE)) {
			textWidth += textScale * useFont->glyphScale * useFont->glyphs[ (const unsigned char)*( buff + len - 1 ) ].xSkip;
		}
	}

	return idMath::Ftoi( rectDraw.w / charSkip );
}