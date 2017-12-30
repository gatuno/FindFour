/*
 * sdl2_rect.h
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

#ifndef __SDL2_RECT_H__
#define __SDL2_RECT_H__

#include <SDL.h>

#ifndef SDL_FALSE
#define SDL_FALSE 0
#endif

#ifndef SDL_TRUE
#define SDL_TRUE -1
#endif

int SDL_RectEmpty(const SDL_Rect *r);
int SDL_HasIntersection(const SDL_Rect * A, const SDL_Rect * B);
int SDL_IntersectRect(const SDL_Rect * A, const SDL_Rect * B, SDL_Rect * result);
void SDL_UnionRect(const SDL_Rect * A, const SDL_Rect * B, SDL_Rect * result);

#endif /* __SDL2_RECT_H__ */
