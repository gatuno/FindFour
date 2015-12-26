/*
 * draw-text.c
 * This file is part of Find Four
 *
 * Copyright (C) 2015 - Félix Arreola Rodríguez
 *
 * Find Four is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Find Four is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Find Four. If not, see <http://www.gnu.org/licenses/>.
 */

#include <SDL.h>
#include <SDL_ttf.h>

#include "draw-text.h"

SDL_Surface *draw_text_with_shadow (TTF_Font *font, int outline, const char *text, SDL_Color foreground, SDL_Color background) {
	SDL_Surface *black_letters, *white_letters;
	SDL_Rect rect;
	
	/* Algunas validaciones */
	if (!font) {
		return NULL;
	}
	
	if (!text || text[0] == '\0') {
		/* Texto vacio */
		return NULL;
	}
	
	TTF_SetFontOutline (font, outline);
	black_letters = TTF_RenderUTF8_Blended (font, text, background);
	
	TTF_SetFontOutline (font, 0);
	white_letters = TTF_RenderUTF8_Blended (font, text, foreground);
	
	rect.w = white_letters->w; rect.h = white_letters->h;
	rect.x = rect.y = outline;
	
	SDL_BlitSurface (white_letters, NULL, black_letters, &rect);
	
	SDL_FreeSurface (white_letters);
	
	return black_letters;
}
