/*
 * findfour.c
 * This file is part of Find Four
 *
 * Copyright (C) 2013 - Félix Arreola Rodríguez
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* ----------------
 * LEGAL NOTICE
 * ----------------
 *
 * This game is NOT related to Club Penguin in any way. Also,
 * this game is not intended to infringe copyrights, the graphics and
 * sounds used are Copyright of Disney.
 *
 * The new SDL code is written by Gatuno, and is released under
 * the term of the GNU General Public License.
 */

#include <stdlib.h>
#include <stdio.h>

#include <SDL.h>
#include <SDL_image.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define FPS (1000/24)

#define SWAP(a, b, t) ((t) = (a), (a) = (b), (b) = (t))
#define RANDOM(x) ((int) (x ## .0 * rand () / (RAND_MAX + 1.0)))

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE !FALSE
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
	
	NUM_IMAGES
};

const char *images_names[NUM_IMAGES] = {
	GAMEDATA_DIR "images/lodge.png",
	
	GAMEDATA_DIR "images/ventana.png",
	GAMEDATA_DIR "images/tablero.png",
	GAMEDATA_DIR "images/coinblue.png",
	GAMEDATA_DIR "images/coinred.png",
	
	GAMEDATA_DIR "images/bigcoinblue.png",
	GAMEDATA_DIR "images/bigcoinred.png"
};

/* TODO: Listar aquí los automátas */

/* Codigos de salida */
enum {
	GAME_NONE = 0, /* No usado */
	GAME_CONTINUE,
	GAME_QUIT
};

/* Estructuras */
typedef struct _Juego {
	int id;
	
	int x, y;
	int w, h;
	
	struct _Juego *prev, *next;
	
	int turno;
	int resalte;
} Juego;

/* Prototipos de función */
int game_loop (void);
void setup (void);
SDL_Surface * set_video_mode(unsigned);

/* Variables globales */
SDL_Surface * screen;
SDL_Surface * images [NUM_IMAGES];

Juego *primero, *ultimo;
static int id = 1;

int main (int argc, char *argv[]) {
	
	setup ();
	
	do {
		if (game_loop () == GAME_QUIT) break;
	} while (1 == 0);
	
	SDL_Quit ();
	return EXIT_SUCCESS;
}

int game_loop (void) {
	int done = 0;
	SDL_Event event;
	SDLKey key;
	Uint32 last_time, now_time;
	SDL_Rect rect;
	
	int g;
	int start = 0;
	int x, y, drag_x, drag_y;
	int manejado;
	
	primero = NULL;
	ultimo = NULL;
	Juego *ventana, *drag;
	
	//SDL_EventState (SDL_MOUSEMOTION, SDL_IGNORE);
	
	drag = NULL;
	
	do {
		last_time = SDL_GetTicks ();
		
		while (SDL_PollEvent(&event) > 0) {
			switch (event.type) {
				case SDL_KEYDOWN:
					if (event.key.keysym.sym == SDLK_n) {
						/* Crear una nueva ventana */
						ventana = (Juego *) malloc (sizeof (Juego));
						
						ventana->id = id++;
						
						ventana->prev = NULL;
						ventana->next = primero;
						ventana->w = images[IMG_WINDOW]->w;
						ventana->h = images[IMG_WINDOW]->h;
						ventana->turno = RANDOM(2);
						ventana->resalte = -1;
						start += 20;
						ventana->x = ventana->y = start;
						
						if (primero == NULL) {
							primero = ultimo = ventana;
						} else {
							primero->prev = ventana;
							primero = ventana;
						}
					}
					break;
				case SDL_MOUSEBUTTONDOWN:
					/* Primero, analizar si el evento cae dentro de alguna ventana */
					
					if (event.button.button != SDL_BUTTON_LEFT) break;
					manejado = FALSE;
					for (ventana = primero; ventana != NULL && manejado == FALSE; ventana = ventana->next) {
						x = event.button.x;
						y = event.button.y;
						if (x >= ventana->x && x < ventana->x + ventana->w && y >= ventana->y && y < ventana->y + ventana->h) {
							x -= ventana->x;
							y -= ventana->y;
							
							if (x >= 64 && x < 168 && y < 22) {
								printf ("Click por el agarre\n");
								/* Click por el agarre */
								drag_x = x;
								drag_y = y;
								
								drag = ventana;
								manejado = TRUE;
							} else if (y >= 16) {
								/* El evento cae dentro de la ventana */
								manejado = TRUE;
							}
							
							if (manejado && primero != ventana) {
								/* Si el evento cayó dentro de esta ventana, levantar la ventana */
								
								/* Desligar completamente */
								if (ventana->prev != NULL) {
									ventana->prev->next = ventana->next;
								} else {
									primero = ventana->next;
								}
								
								if (ventana->next != NULL) {
									ventana->next->prev = ventana->prev;
								} else {
									ultimo = ventana->prev;
								}
								
								/* Levantar la ventana a primer plano */
								ventana->next = primero;
								primero->prev = ventana;
								ventana->prev = NULL;
								primero = ventana;
							}
						}
					}
					
					break;
				case SDL_MOUSEMOTION:
					if (drag != NULL) {
						/* Mover la ventana a las coordenadas del mouse - los offsets */
						drag->x = event.motion.x - drag_x;
						drag->y = event.motion.y - drag_y;
					} else {
						/* En caso contrario, buscar por un evento de ficha de turno */
						manejado = FALSE;
						for (ventana = primero; ventana != NULL; ventana = ventana->next) {
							x = event.motion.x;
							y = event.motion.y;
							ventana->resalte = -1;
							if (manejado == FALSE && x >= ventana->x && x < ventana->x + ventana->w && y >= ventana->y && y < ventana->y + ventana->h) {
								manejado = TRUE;
								x -= ventana->x;
								y -= ventana->y;
								
								if (y > 65 && y < 217 && x > 26 && x < 208) {
									/* Está dentro del tablero */
									if (x >= 32 && x < 56) {
										/* Primer fila de resalte */
										ventana->resalte = 0;
									} else if (x >= 56 && x < 81) {
										ventana->resalte = 1;
									} else if (x >= 81 && x < 105) {
										ventana->resalte = 2;
									} else if (x >= 105 && x < 129) {
										ventana->resalte = 3;
									} else if (x >= 129 && x < 153) {
										ventana->resalte = 4;
									} else if (x >= 153 && x < 178) {
										ventana->resalte = 5;
									} else if (x >= 178 && x < 206) {
										ventana->resalte = 6;
									}
								}
							}
						}
					}
					break;
				case SDL_MOUSEBUTTONUP:
					drag = NULL;
					break;
				case SDL_QUIT:
					/* Vamos a cerrar la aplicación */
					done = GAME_QUIT;
					break;
			}
		}
		
		SDL_BlitSurface (images[IMG_LODGE], NULL, screen, NULL);
		//printf ("Dibujar: \n");
		for (ventana = ultimo; ventana != NULL; ventana = ventana->prev) {
			//printf ("\tVentana con ID: %i\n", ventana->id);
			rect.x = ventana->x;
			rect.y = ventana->y;
			rect.w = ventana->w;
			rect.h = ventana->h;
			
			SDL_BlitSurface (images[IMG_WINDOW], NULL, screen, &rect);
			
			rect.x = ventana->x + 26;
			rect.y = ventana->y + 65;
			rect.w = images[IMG_BOARD]->w;
			rect.h = images[IMG_BOARD]->h;
			
			SDL_BlitSurface (images[IMG_BOARD], NULL, screen, &rect);
			
			if (ventana->resalte >= 0) {
				/* Dibujar la ficha resalta en la columna correspondiente */
				rect.x = ventana->x;
				rect.y = ventana->y + 46;
				switch (ventana->resalte) {
					case 0:
						rect.x += 30;
						break;
					case 1:
						rect.x += 53;
						break;
					case 2:
						rect.x += 79;
						break;
					case 3:
						rect.x += 102;
						break;
					case 4:
						rect.x += 126;
						break;
					case 5:
						rect.x += 150;
						break;
					case 6:
						rect.x += 175;
						break;
				}
				rect.w = images[IMG_BIGCOINRED]->w;
				rect.h = images[IMG_BIGCOINRED]->h;
				
				if (ventana->turno % 2 == 0) {
					SDL_BlitSurface (images[IMG_BIGCOINRED], NULL, screen, &rect);
				} else {
					SDL_BlitSurface (images[IMG_BIGCOINBLUE], NULL, screen, &rect);
				}
			}
		}
		
		SDL_Flip (screen);
		
		now_time = SDL_GetTicks ();
		if (now_time < last_time + FPS) SDL_Delay(last_time + FPS - now_time);
		
	} while (!done);
	
	return done;
}

/* Set video mode: */
/* Mattias Engdegard <f91-men@nada.kth.se> */
SDL_Surface * set_video_mode (unsigned flags) {
	/* Prefer 16bpp, but also prefer native modes to emulated 16bpp. */

	int depth;

	depth = SDL_VideoModeOK (760, 480, 16, flags);
	return depth ? SDL_SetVideoMode (760, 480, depth, flags) : NULL;
}

void setup (void) {
	SDL_Surface * image;
	int g;
	
	/* Inicializar el Video SDL */
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		fprintf (stderr,
			"Error: Can't initialize the video subsystem\n"
			"The error returned by SDL is:\n"
			"%s\n", SDL_GetError());
		exit (1);
	}
	
	/* Crear la pantalla de dibujado */
	screen = set_video_mode (0);
	
	if (screen == NULL) {
		fprintf (stderr,
			"Error: Can't setup 760x480 video mode.\n"
			"The error returned by SDL is:\n"
			"%s\n", SDL_GetError());
		exit (1);
	}
	
	for (g = 0; g < NUM_IMAGES; g++) {
		image = IMG_Load (images_names[g]);
		
		if (image == NULL) {
			fprintf (stderr,
				"Failed to load data file:\n"
				"%s\n"
				"The error returned by SDL is:\n"
				"%s\n", images_names[g], SDL_GetError());
			SDL_Quit ();
			exit (1);
		}
		
		images[g] = image;
		/* TODO: Mostrar la carga de porcentaje */
	}
	
	srand (SDL_GetTicks ());
}

