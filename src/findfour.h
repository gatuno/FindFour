/*
 * findfour.h
 * This file is part of Find Four
 *
 * Copyright (C) 2015 - Félix Arreola Rodríguez
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __FINDFOUR_H__
#define __FINDFOUR_H__

#include <SDL.h>

#define NICK_SIZE 16

#define RANDOM(x) ((int) (x ## .0 * rand () / (RAND_MAX + 1.0)))

#ifndef FALSE
#	define FALSE 0
#endif

#ifndef TRUE
#	define TRUE !FALSE
#endif

/* Enumerar las imágenes */
enum {
	IMG_LODGE,
	
	IMG_WINDOW,
	
	IMG_BOARD,
	IMG_COINBLUE,
	IMG_COINRED,
	
	IMG_BIGCOINBLUE,
	IMG_BIGCOINRED,
	
	IMG_WINDOW_CHAT,
	
	IMG_SHADOW_UP,
	IMG_SHADOW_OVER,
	IMG_SHADOW_DOWN,
	
	IMG_BUTTON_CLOSE_UP,
	IMG_BUTTON_CLOSE_OVER,
	IMG_BUTTON_CLOSE_DOWN,
	
	IMG_BUTTON_ARROW_1_UP,
	IMG_BUTTON_ARROW_1_OVER,
	IMG_BUTTON_ARROW_1_DOWN,
	
	IMG_BUTTON_ARROW_2_UP,
	IMG_BUTTON_ARROW_2_OVER,
	IMG_BUTTON_ARROW_2_DOWN,
	
	IMG_BUTTON_LIST_UP,
	IMG_BUTTON_LIST_OVER,
	IMG_BUTTON_LIST_DOWN,
	
	IMG_LIST_MINI,
	
	IMG_LIST_BIG,
	
	IMG_WIN_1,
	IMG_WIN_2,
	IMG_WIN_3,
	IMG_WIN_4,
	
	NUM_IMAGES
};

enum {
	WINDOW_GAME,
	WINDOW_CHAT,
	
	NUM_WINDOWS
};

typedef struct _Ventana {
	int tipo;
	
	/* Coordenadas de la ventana */
	int x, y;
	int w, h;
	
	/* Para la lista ligada */
	struct _Ventana *prev, *next;
	
	int mostrar;
	
	/* Manejadores de la ventana */
	int (*mouse_down)(struct _Ventana *, int, int, int **);
	int (*mouse_motion)(struct _Ventana *, int, int, int **);
	int (*mouse_up)(struct _Ventana *, int, int, int **);
	void (*draw)(struct _Ventana *, SDL_Surface *);
} Ventana;

extern Ventana *primero, *ultimo;
extern SDL_Surface * images [NUM_IMAGES];

void start_drag (Ventana *v, int offset_x, int offset_y);
void stop_drag (Ventana *v);

#endif /* __FINDFOUR_H__ */

