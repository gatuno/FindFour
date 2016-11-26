/*
 * inputbox.h
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

#ifndef __INPUTBOX_H__
#define __INPUTBOX_H__

#include "findfour.h"

#define INPUTBOX_CURSOR_RATE 12

typedef struct _InputBox InputBox;

typedef void (*InputBoxFunc) (InputBox *v, const char *texto);

struct _InputBox {
	Ventana ventana;
	
	/* El botón de cierre */
	int close_frame; /* ¿El refresh es necesario? */
	
	char buffer[256];
	
	int send_frame;
	
	int box_y;
	InputBoxFunc callback;
	InputBoxFunc close_callback;
	SDL_Surface *text_s;
	SDL_Surface *text_ask;
	
	int cursor_vel;
};

InputBox *crear_inputbox (InputBoxFunc, const char *, const char *, InputBoxFunc);
void eliminar_inputbox (InputBox *ib);

#endif /* __INPUTBOX_H__ */

