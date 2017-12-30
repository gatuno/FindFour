/*
 * sdl2_rect.c
 * This file is part of Find Four
 *
 * Copyright (C) 2017 - Félix Arreola Rodríguez
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
 * along with Find Four; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */

#include <SDL.h>

#include "sdl2_rect.h"

/* Funciones */
int SDL_RectEmpty(const SDL_Rect *r) {
	return ((!r) || (r->w <= 0) || (r->h <= 0)) ? SDL_TRUE : SDL_FALSE;
}

int SDL_HasIntersection(const SDL_Rect * A, const SDL_Rect * B) {
	int Amin, Amax, Bmin, Bmax;

	if (!A) {
		return SDL_FALSE;
	}

	if (!B) {
		return SDL_FALSE;
	}

	/* Special cases for empty rects */
	if (SDL_RectEmpty(A) || SDL_RectEmpty(B)) {
		return SDL_FALSE;
	}

	/* Horizontal intersection */
	Amin = A->x;
	Amax = Amin + A->w;
	Bmin = B->x;
	Bmax = Bmin + B->w;
	if (Bmin > Amin)
		Amin = Bmin;
	if (Bmax < Amax)
		Amax = Bmax;
	if (Amax <= Amin)
		return SDL_FALSE;

	/* Vertical intersection */
	Amin = A->y;
	Amax = Amin + A->h;
	Bmin = B->y;
	Bmax = Bmin + B->h;
	if (Bmin > Amin)
		Amin = Bmin;
	if (Bmax < Amax)
		Amax = Bmax;
	if (Amax <= Amin)
		return SDL_FALSE;

	return SDL_TRUE;
}

int SDL_IntersectRect(const SDL_Rect * A, const SDL_Rect * B, SDL_Rect * result) {
	int Amin, Amax, Bmin, Bmax;

	if (!A) {
		return SDL_FALSE;
	}

	if (!B) {
		return SDL_FALSE;
	}

	if (!result) {
		return SDL_FALSE;
	}

	/* Special cases for empty rects */
	if (SDL_RectEmpty(A) || SDL_RectEmpty(B)) {
		result->w = 0;
		result->h = 0;
		return SDL_FALSE;
	}

	/* Horizontal intersection */
	Amin = A->x;
	Amax = Amin + A->w;
	Bmin = B->x;
	Bmax = Bmin + B->w;
	if (Bmin > Amin)
		Amin = Bmin;
	result->x = Amin;
	if (Bmax < Amax)
		Amax = Bmax;
	if (Amax - Amin < 0) {
		result->w = 0;
	} else {
		result->w = Amax - Amin;
	}

	/* Vertical intersection */
	Amin = A->y;
	Amax = Amin + A->h;
	Bmin = B->y;
	Bmax = Bmin + B->h;
	if (Bmin > Amin)
		Amin = Bmin;
	result->y = Amin;
	if (Bmax < Amax)
		Amax = Bmax;
	if (Amax - Amin < 0) {
		result->h = 0;
	} else {
		result->h = Amax - Amin;
	}

	return !SDL_RectEmpty(result);
}

void SDL_UnionRect(const SDL_Rect * A, const SDL_Rect * B, SDL_Rect * result) {
	int Amin, Amax, Bmin, Bmax;

	if (!A) {
		return;
	}

	if (!B) {
		return;
	}

	if (!result) {
		return;
	}

	/* Special cases for empty Rects */
	if (SDL_RectEmpty(A)) {
		if (SDL_RectEmpty(B)) {
			/* A and B empty */
			return;
		} else {
			/* A empty, B not empty */
			*result = *B;
			return;
		}
	} else {
		if (SDL_RectEmpty(B)) {
			/* A not empty, B empty */
			*result = *A;
			return;
		}
	}

	/* Horizontal union */
	Amin = A->x;
	Amax = Amin + A->w;
	Bmin = B->x;
	Bmax = Bmin + B->w;
	if (Bmin < Amin)
		Amin = Bmin;
	result->x = Amin;
	if (Bmax > Amax)
		Amax = Bmax;
	result->w = Amax - Amin;

	/* Vertical union */
	Amin = A->y;
	Amax = Amin + A->h;
	Bmin = B->y;
	Bmax = Bmin + B->h;
	if (Bmin < Amin)
		Amin = Bmin;
	result->y = Amin;
	if (Bmax > Amax)
		Amax = Bmax;
	result->h = Amax - Amin;
}
