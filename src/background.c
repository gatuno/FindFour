/*
 * background.c
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
 * along with Find Four; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */

/* Manejador del fondo */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <SDL.h>
#include <SDL_image.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gettext.h"
#define _(string) gettext (string)

#include "findfour.h"
#include "ventana.h"
#include "path.h"
#include "sdl2_rect.h"

/* Cuántos fondos diferentes hay */
enum {
	BACKGROUND_LODGE_NORMAL,
	
	NUM_BACKGROUNDS
};

/* Fondo Logde normal */
enum {
	IMG_LODGE_NORMAL,
	IMG_LODGE_NORMAL_FIRE,
	IMG_LODGE_NORMAL_CANDLE,
	
	NUM_LODGE_NORMAL
};

static const char *lodge_normal_images_names [NUM_LODGE_NORMAL] = {
	"images/lodge.png",
	"images/lodge_fire.png",
	"images/lodge_candle.png",
};

static int background_current;
static int background_counters[5];

static SDL_Surface **background_images = NULL;

void setup_background (void) {
	SDL_Surface * image;
	int g;
	char buffer_file[8192];
	
	int top;
	const char **names;
	
	background_current = BACKGROUND_LODGE_NORMAL;
	
	if (background_current == BACKGROUND_LODGE_NORMAL) {
		top = NUM_LODGE_NORMAL;
		names = lodge_normal_images_names;
		background_counters[0] = background_counters[1] = 0;
		/* background_counters[0] = fire
		 * background_counters[1] = candle
		 */
	}
	
	background_images = (SDL_Surface **) malloc (sizeof (SDL_Surface *) * top);
	if (background_images == NULL) {
		fprintf (stderr,
			_("Failed to reserve memory\n"));
		SDL_Quit ();
		exit (1);
	}
	
	for (g = 0; g < top; g++) {
		sprintf (buffer_file, "%s%s", systemdata_path, names[g]);
		image = IMG_Load (buffer_file);
		
		if (image == NULL) {
			fprintf (stderr,
				_("Failed to load data file:\n"
				"%s\n"
				"The error returned by SDL is:\n"
				"%s\n"), buffer_file, SDL_GetError());
			SDL_Quit ();
			exit (1);
		}
		
		background_images[g] = image;
		/* TODO: Mostrar la carga de porcentaje */
	}
}

void draw_background (SDL_Surface *screen, SDL_Rect *update_rect) {
	SDL_Rect rect, rect2, origen, whole_rect;
	int g;
	
	if (update_rect == NULL) {
		whole_rect.x = whole_rect.y = 0;
		whole_rect.w = 760;
		whole_rect.h = 480;
		
		update_rect = &whole_rect;
	}
	
	SDL_BlitSurface (background_images[IMG_LODGE_NORMAL], update_rect, screen, update_rect);
	
	rect.x = 638;
	rect.y = 217;
	rect.w = 47;
	rect.h = 42;
	
	if (SDL_IntersectRect (&rect, update_rect, &rect2) && (background_counters[0] / 2) > 0) {
		origen = rect2;
		origen.x -= rect.x;
		origen.y -= rect.y;
		
		/* Dibujar la parte de la fogata en el frame correcto */
		origen.x += ((background_counters[0] / 2) - 1) * 47;
		
		SDL_BlitSurface (background_images[IMG_LODGE_NORMAL_FIRE], &origen, screen, &rect2);
	}
	
	rect.x = 254;
	rect.y = 95;
	rect.w = 15;
	rect.h = 42;
	
	if (background_counters[1] < 30) {
		g = background_counters[1] / 2;
	} else if (background_counters[1] >= 30 && background_counters[1] < 38) {
		g = 15;
	} else {
		g = 16 + (background_counters[1] - 38) / 2;
	}
	
	if (SDL_IntersectRect (&rect, update_rect, &rect2) && g > 0) {
		origen = rect2;
		origen.x -= rect.x;
		origen.y -= rect.y;
		
		origen.x += (g - 1) * 15;
	
		SDL_BlitSurface (background_images[IMG_LODGE_NORMAL_CANDLE], &origen, screen, &rect2);
	}
}

void update_background (SDL_Surface *screen) {
	int g, h;
	SDL_Rect rect, rect2;
	
	if (background_current == BACKGROUND_LODGE_NORMAL) {
		/* La fogata del lodge */
		background_counters[0]++;
		if (background_counters[0] >= 10) background_counters[0] = 0;
		
		if (background_counters[0] % 2 == 0) {
			/* Toca actualización de la fogata */
			rect.w = 47;
			rect.h = 42;
			
			rect.x = 638;
			rect.y = 217;
			window_manager_background_update (&rect);
		}
		
		/* La vela del lodge */
		if (background_counters[1] == 13) {
			background_counters[1] = 0;
		} else if (background_counters[1] == 41) {
			background_counters[1] = 4;
		} else {
			background_counters[1]++;
		}
		
		rect.x = 254;
		rect.y = 95;
		rect.w = 15;
		rect.h = 42;
		if (background_counters[1] < 30 && background_counters[1] % 2 == 0) {
			/* Actualización */
			window_manager_background_update (&rect);
		} else if (background_counters[1] == 30) {
			/* Actualización */
			window_manager_background_update (&rect);
		} else if (background_counters[1] >= 38 && background_counters[1] % 2 == 0) {
			/* Actualización */
			window_manager_background_update (&rect);
		}
	}
}

void background_mouse_motion (int x, int y) {
	if (background_current == BACKGROUND_LODGE_NORMAL) {
		/* Si el movimiento del mouse no entró a alguna ventana, es para el fondo */
		if (x >= 253 && x < 272 && y >= 117 && y < 179 && background_counters[1] < 14) {
			background_counters[1] = 14;
		}
	}
}
