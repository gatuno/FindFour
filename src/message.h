/*
 * message.h
 * This file is part of Find Four
 *
 * Copyright (C) 2016 - Félix Arreola Rodríguez
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

#ifndef __MESSAGE_H__
#define __MESSAGE_H__

#include <stdarg.h>

#include <SDL.h>

enum {
	MSG_NORMAL,
	MSG_ERROR
};

typedef struct _Message Message;

extern Message *list_msg;

void message_add (int tipo, const char *texto_boton, const char *cadena, ...);
void message_display (SDL_Surface *screen);

int message_mouse_down (int, int, int **);
int message_mouse_motion (int, int, int **);
int message_mouse_up (int, int, int **);

#endif /* __MESSAGE_H__ */

