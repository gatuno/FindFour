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

#include "findfour.h"
#include "path.h"

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
			"Failed to reserve memory\n");
		SDL_Quit ();
		exit (1);
	}
	
	for (g = 0; g < top; g++) {
		sprintf (buffer_file, "%s%s", systemdata_path, names[g]);
		image = IMG_Load (buffer_file);
		
		if (image == NULL) {
			fprintf (stderr,
				"Failed to load data file:\n"
				"%s\n"
				"The error returned by SDL is:\n"
				"%s\n", buffer_file, SDL_GetError());
			SDL_Quit ();
			exit (1);
		}
		
		background_images[g] = image;
		/* TODO: Mostrar la carga de porcentaje */
	}
}

void draw_background (SDL_Surface *screen) {
	int g;
	SDL_Rect rect, rect2;
	
	if (background_current == BACKGROUND_LODGE_NORMAL) {
		SDL_BlitSurface (background_images[IMG_LODGE_NORMAL], NULL, screen, NULL);
		
		/* La fogata del lodge */
		g = background_counters[0] / 2;
		if (g > 0) {
			rect2.x = (g - 1) * 47;
			rect2.y = 0;
			rect.w = rect2.w = 47;
			rect.h = rect2.h = 42;
		
			rect.x = 638;
			rect.y = 217;
			SDL_BlitSurface (background_images[IMG_LODGE_NORMAL_FIRE], &rect2, screen, &rect);
		}
		background_counters[0]++;
		if (background_counters[0] >= 10) background_counters[0] = 0;
	
		/* La vela del lodge */
		if (background_counters[1] < 30) {
			g = background_counters[1] / 2;
		} else if (background_counters[1] >= 30 && background_counters[1] < 38) {
			g = 15;
		} else {
			g = 16 + (background_counters[1] - 38) / 2;
		}
	
		if (g > 0) {
			rect2.x = (g - 1) * 15;
			rect2.y = 0;
			rect2.w = rect.w = 15;
			rect2.h = rect.h = 42;
		
			rect.x = 254;
			rect.y = 95;
			SDL_BlitSurface (background_images[IMG_LODGE_NORMAL_CANDLE], &rect2, screen, &rect);
		}
	
		if (background_counters[1] == 13) {
			background_counters[1] = 0;
		} else if (background_counters[1] == 41) {
			background_counters[1] = 4;
		} else {
			background_counters[1]++;
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