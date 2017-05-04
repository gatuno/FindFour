/*
 * ventana.h
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

#ifndef __VENTANA_H__
#define __VENTANA_H__

#include <SDL.h>

#include "findfour.h"

enum {
	BUTTON_NORMAL = 0,
	BUTTON_OVER,
	BUTTON_DOWN,
	
	BUTTON_FRAMES
};

typedef struct _Ventana Ventana;

typedef int (*FindWindowMouseFunc)(Ventana *, int, int);
typedef int (*FindWindowKeyFunc)(Ventana *, SDL_KeyboardEvent *);
typedef void (*FindWindowButtonFrameChange)(Ventana *, int button, int frame);
typedef void (*FindWindowButtonEvent)(Ventana *, int button);
Ventana *window_create (int w, int h, int top_window);

void window_set_data (Ventana *v, void *);
void * window_get_data (Ventana *v);
SDL_Surface * window_get_surface (Ventana *v);

void window_want_redraw (Ventana *v);

void window_hide (Ventana *v);
void window_show (Ventana *v);

void window_register_mouse_events (Ventana *v, FindWindowMouseFunc down, FindWindowMouseFunc motion, FindWindowMouseFunc up);
void window_register_keyboard_events (Ventana *v, FindWindowKeyFunc down, FindWindowKeyFunc up);

void window_register_buttons (Ventana *v, int count, FindWindowButtonFrameChange frame, FindWindowButtonEvent event);

void window_button_mouse_down (Ventana *v, int button);
void window_button_mouse_motion (Ventana *v, int button);
void window_button_mouse_up (Ventana *v, int button);

void window_start_drag (Ventana *v, int offset_x, int offset_y);
void window_cancel_draging (void);

void window_destroy (Ventana *v);

void window_manager_event (SDL_Event event);
void window_manager_draw (SDL_Surface *screen);

#endif /* __VENTANA_H__ */

