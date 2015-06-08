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

#include <string.h>

#include <SDL.h>
#include <SDL_image.h>

/* Para el manejo de red */
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

/* Para el tipo de dato uint8_t */
#include <stdint.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "findfour.h"
#include "juego.h"
#include "netplay.h"
#include "cp-button.h"
#include "chat.h"

#define FPS (1000/24)

#define SWAP(a, b, t) ((t) = (a), (a) = (b), (b) = (t))

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE !FALSE
#endif

const char *images_names[NUM_IMAGES] = {
	GAMEDATA_DIR "images/lodge.png",
	
	GAMEDATA_DIR "images/ventana.png",
	GAMEDATA_DIR "images/tablero.png",
	GAMEDATA_DIR "images/coinblue.png",
	GAMEDATA_DIR "images/coinred.png",
	
	GAMEDATA_DIR "images/bigcoinblue.png",
	GAMEDATA_DIR "images/bigcoinred.png",
	
	GAMEDATA_DIR "images/ventana-chat.png",
	
	GAMEDATA_DIR "images/shadow-up.png",
	GAMEDATA_DIR "images/shadow-over.png",
	GAMEDATA_DIR "images/shadow-down.png",
	
	GAMEDATA_DIR "images/button-up.png",
	GAMEDATA_DIR "images/button-over.png",
	GAMEDATA_DIR "images/button-down.png",
	
	GAMEDATA_DIR "images/button-arrow-1-up.png",
	GAMEDATA_DIR "images/button-arrow-1-over.png",
	GAMEDATA_DIR "images/button-arrow-1-down.png",
	
	GAMEDATA_DIR "images/button-arrow-2-up.png",
	GAMEDATA_DIR "images/button-arrow-2-over.png",
	GAMEDATA_DIR "images/button-arrow-2-down.png",
	
	GAMEDATA_DIR "images/button-list-up.png",
	GAMEDATA_DIR "images/button-list-over.png",
	GAMEDATA_DIR "images/button-list-down.png",
	
	GAMEDATA_DIR "images/list-mini.png"
};

/* TODO: Listar aquí los automátas */

/* Codigos de salida */
enum {
	GAME_NONE = 0, /* No usado */
	GAME_CONTINUE,
	GAME_QUIT
};

/* Prototipos de función */
int game_loop (void);
void setup (void);
SDL_Surface * set_video_mode(unsigned);

/* Variables globales */
SDL_Surface * screen;
SDL_Surface * images [NUM_IMAGES];

Ventana *primero, *ultimo, *drag;
int drag_x, drag_y;

int client_port, server_port;

int main (int argc, char *argv[]) {
	
	setup ();
	client_port = 3301;
	server_port = 3300;
	
	if (argc > 2) {
		client_port = atoi (argv[1]);
		server_port = atoi (argv[2]);
	}
	
	printf ("Puerto cliente: %i, conectar al puerto: %i\n", client_port, server_port);
	
	cp_button_start ();
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
	int *map = NULL;
	Chat *chat;
	
	int g, h;
	int start = 0;
	int x, y;
	int manejado;
	
	/* Para la red */
	int fd_sock;
	
	primero = NULL;
	ultimo = NULL;
	Ventana *ventana;
	
	//SDL_EventState (SDL_MOUSEMOTION, SDL_IGNORE);
	
	drag = NULL;
	
	fd_sock = findfour_netinit (server_port);
	
	if (fd_sock < 0) {
		/* FIXME: Mostrar ventana de error */
		
		return GAME_QUIT;
	}
	
	chat = inicializar_chat ();
	
	do {
		last_time = SDL_GetTicks ();
		
		/* Antes de procesar los eventos locales, procesar la red */
		process_netevent ();
		
		while (SDL_PollEvent(&event) > 0) {
			switch (event.type) {
				case SDL_KEYDOWN:
					if (event.key.keysym.sym == SDLK_n) {
						/* Crear una nueva ventana y conectar */
						ventana = (Ventana *) crear_juego ();
						
						conectar_con ((Juego *) ventana, "Gatuno Cliente", "127.0.0.1", client_port);
					} else if (event.key.keysym.sym == SDLK_c) {
						chat->ventana.mostrar = TRUE;
					}
					break;
				case SDL_MOUSEBUTTONDOWN:
					/* Primero, analizar si el evento cae dentro de alguna ventana */
					
					if (event.button.button != SDL_BUTTON_LEFT) break;
					map = NULL;
					manejado = FALSE;
					x = event.button.x;
					y = event.button.y;
					
					for (ventana = primero; ventana != NULL && manejado == FALSE; ventana = ventana->next) {
						/* No hay eventos para ventanas que se están cerrando */
						if (!ventana->mostrar) continue;
						
						/* Si el evento hace match por las coordenadas, preguntarle a la ventana si lo quiere manejar */
						if (x >= ventana->x && x < ventana->x + ventana->w && y >= ventana->y && y < ventana->y + ventana->h) {
							/* Pasarlo al manejador de la ventana */
							if (ventana->mouse_down != NULL) {
								manejado = ventana->mouse_down (ventana, x - ventana->x, y - ventana->y, &map);
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
					
					cp_button_down (map);
					break;
				case SDL_MOUSEMOTION:
					map = NULL; /* Para los botones de cierre */
					if (drag != NULL) {
						/* Mover la ventana a las coordenadas del mouse - los offsets */
						drag->x = event.motion.x - drag_x;
						drag->y = event.motion.y - drag_y;
					} else {
						/* En caso contrario, buscar por un evento de ficha de turno */
						manejado = FALSE;
						
						x = event.motion.x;
						y = event.motion.y;
						for (ventana = primero; ventana != NULL && manejado == FALSE; ventana = ventana->next) {
							/* Si el evento hace match por las coordenadas, preguntarle a la ventana si lo quiere manejar */
							if (x >= ventana->x && x < ventana->x + ventana->w && y >= ventana->y && y < ventana->y + ventana->h) {
								if (ventana->mouse_motion != NULL) {
									manejado = ventana->mouse_motion (ventana, x - ventana->x, y - ventana->y, &map);
								}
							}
						}
						
						/* Recorrer las ventanas restantes y mandarles un evento mouse motion nulo */
						while (ventana != NULL) {
							if (ventana->mouse_motion != NULL) {
								manejado = ventana->mouse_motion (ventana, -1, -1, NULL);
							}
							
							ventana = ventana->next;
						}
					}
					
					cp_button_motion (map);
					break;
				case SDL_MOUSEBUTTONUP:
					if (event.button.button != SDL_BUTTON_LEFT) break;
					drag = NULL;
					manejado = FALSE;
					map = NULL;
					x = event.button.x;
					y = event.button.y;
					for (ventana = primero; ventana != NULL && manejado == FALSE; ventana = ventana->next) {
						/* Si el evento hace match por las coordenadas, preguntarle a la ventana si lo quiere manejar */
						if (x >= ventana->x && x < ventana->x + ventana->w && y >= ventana->y && y < ventana->y + ventana->h) {
							if (ventana->mouse_up != NULL) {
								manejado = ventana->mouse_up (ventana, x - ventana->x, y - ventana->y, &map);
							}
						}
					}
					
					if (map == NULL) {
						cp_button_up (NULL);
					}
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
			if (!ventana->mostrar) continue;
			
			ventana->draw (ventana, screen);
		}
		
		SDL_Flip (screen);
		
		now_time = SDL_GetTicks ();
		if (now_time < last_time + FPS) SDL_Delay(last_time + FPS - now_time);
		
	} while (!done);
	
	return done;
}

void start_drag (Ventana *v, int offset_x, int offset_y) {
	/* Cuando una ventana determina que se va a mover, guardamos su offset para un arrastre perfecto */
	drag_x = offset_x;
	drag_y = offset_y;
	
	drag = v;
}

void stop_drag (Ventana *v) {
	if (drag == v) drag = NULL;
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

