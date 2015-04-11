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

#define FPS (1000/24)

#define SWAP(a, b, t) ((t) = (a), (a) = (b), (b) = (t))

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

/* Prototipos de función */
int game_loop (void);
void setup (void);
SDL_Surface * set_video_mode(unsigned);

/* Variables globales */
SDL_Surface * screen;
SDL_Surface * images [NUM_IMAGES];

Juego *primero, *ultimo;
static int id = 1;
const int tablero_cols[7] = {32, 56, 81, 105, 129, 153, 178};
const int tablero_filas[6] = {69, 93, 117, 141, 166, 190};

int main (int argc, char *argv[]) {
	
	setup ();
	
	do {
		if (game_loop () == GAME_QUIT) break;
	} while (1 == 0);
	
	SDL_Quit ();
	return EXIT_SUCCESS;
}

Juego *crear_ventana (void) {
	Juego *ventana;
	static int start = 0;
	
	/* Crear una nueva ventana */
	ventana = (Juego *) malloc (sizeof (Juego));
	
	ventana->prev = NULL;
	ventana->next = primero;
	ventana->w = 232; /* FIXME: Arreglar esto */
	ventana->h = 324;
	ventana->turno = 0;
	ventana->inicio = RANDOM(2);
	ventana->resalte = -1;
	start += 20;
	ventana->x = ventana->y = start;
	
	/* Vaciar el tablero */
	memset (ventana->tablero, 0, sizeof (int[6][7]));
	
	if (primero == NULL) {
		primero = ultimo = ventana;
	} else {
		primero->prev = ventana;
		primero = ventana;
	}
	
	return ventana;
}

int game_loop (void) {
	int done = 0;
	SDL_Event event;
	SDLKey key;
	Uint32 last_time, now_time;
	SDL_Rect rect;
	
	int g, h;
	int start = 0;
	int x, y, drag_x, drag_y;
	int manejado;
	
	/* Para la red */
	int fd_sock;
	
	primero = NULL;
	ultimo = NULL;
	Juego *ventana, *drag;
	
	//SDL_EventState (SDL_MOUSEMOTION, SDL_IGNORE);
	
	drag = NULL;
	
	fd_sock = findfour_netinit ();
	
	if (fd_sock < 0) {
		/* Mostrar ventana de error */
		
		return GAME_QUIT;
	}
	
	do {
		last_time = SDL_GetTicks ();
		
		/* Antes de procesar los eventos locales, procesar la red */
		process_netevent (fd_sock);
		
		while (SDL_PollEvent(&event) > 0) {
			switch (event.type) {
				case SDL_KEYDOWN:
					if (event.key.keysym.sym == SDLK_n) {
						/* Crear una nueva ventana y conectar */
						ventana = crear_ventana ();
						
						/* Cambiar el turno inicial a -1 hasta que el otro extremo nos asigne turno */
						ventana->inicio = -1;
						
						/* Temporalmente asignar la IP */
						struct sockaddr_in6 *bind_addr = (struct sockaddr_in6 *) &ventana->cliente;
						
						bind_addr->sin6_family = AF_INET6;
						bind_addr->sin6_port = htons (CLIENT_PORT);
						inet_pton (AF_INET6, "::1", &bind_addr->sin6_addr);
						
						ventana->tamsock = sizeof (ventana->cliente);
						
						enviar_syn (fd_sock, ventana, "Gatuno Cliente");
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
								//printf ("Click por el agarre\n");
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
						for (ventana = primero; ventana != NULL && manejado == FALSE; ventana = ventana->next) {
							x = event.motion.x;
							y = event.motion.y;
							ventana->resalte = -1;
							if (x >= ventana->x && x < ventana->x + ventana->w && y >= ventana->y && y < ventana->y + ventana->h) {
								manejado = TRUE;
								x -= ventana->x;
								y -= ventana->y;
								
								/* Si es nuestro turno, hacer resalte */
								if (y > 65 && y < 217 && x > 26 && x < 208 && (ventana->turno % 2) == ventana->inicio) {
									/* Está dentro del tablero */
									if (x >= 32 && x < 56 && ventana->tablero[0][0] == 0) {
										/* Primer fila de resalte */
										ventana->resalte = 0;
									} else if (x >= 56 && x < 81 && ventana->tablero[0][1] == 0) {
										ventana->resalte = 1;
									} else if (x >= 81 && x < 105 && ventana->tablero[0][2] == 0) {
										ventana->resalte = 2;
									} else if (x >= 105 && x < 129 && ventana->tablero[0][3] == 0) {
										ventana->resalte = 3;
									} else if (x >= 129 && x < 153 && ventana->tablero[0][4] == 0) {
										ventana->resalte = 4;
									} else if (x >= 153 && x < 178 && ventana->tablero[0][5] == 0) {
										ventana->resalte = 5;
									} else if (x >= 178 && x < 206 && ventana->tablero[0][6] == 0) {
										ventana->resalte = 6;
									}
								}
							}
						}
						
						/* Recorrer las ventanas restantes y quitarles el resaltado */
						while (ventana != NULL) {
							ventana->resalte = -1;
							
							ventana = ventana->next;
						}
					}
					break;
				case SDL_MOUSEBUTTONUP:
					drag = NULL;
					if (event.button.button != SDL_BUTTON_LEFT) break;
					manejado = FALSE;
					for (ventana = primero; ventana != NULL && manejado == FALSE; ventana = ventana->next) {
						x = event.button.x;
						y = event.button.y;
						if (x >= ventana->x && x < ventana->x + ventana->w && y >= ventana->y && y < ventana->y + ventana->h) {
							x -= ventana->x;
							y -= ventana->y;
							
							if ((y >= 16) || (x >= 64 && x < 168 && y < 22)) {
								/* El evento cae dentro de la ventana */
								manejado = TRUE;
							}
							if (y > 65 && y < 217 && x > 26 && x < 208 && (ventana->turno % 2) == ventana->inicio) {
								/* Está dentro del tablero */
								h = -1;
								if (x >= 32 && x < 56 && ventana->tablero[0][0] == 0) {
									/* Primer fila de resalte */
									h = 0;
								} else if (x >= 56 && x < 81 && ventana->tablero[0][1] == 0) {
									h = 1;
								} else if (x >= 81 && x < 105 && ventana->tablero[0][2] == 0) {
									h = 2;
								} else if (x >= 105 && x < 129 && ventana->tablero[0][3] == 0) {
									h = 3;
								} else if (x >= 129 && x < 153 && ventana->tablero[0][4] == 0) {
									h = 4;
								} else if (x >= 153 && x < 178 && ventana->tablero[0][5] == 0) {
									h = 5;
								} else if (x >= 178 && x < 206 && ventana->tablero[0][6] == 0) {
									h = 6;
								}
								
								if (h >= 0) {
									g = 5;
									while (g > 0 && ventana->tablero[g][h] != 0) g--;
									
									/* Poner la ficha en la posición [g][h] y avanzar turno */
									ventana->tablero[g][h] = (ventana->turno % 2) + 1;
									/* Enviar el turno */
									enviar_movimiento (fd_sock, ventana, h);
									ventana->resalte = -1;
								}
							}
						}
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
			//printf ("\tVentana con ID: %i\n", ventana->id);
			rect.x = ventana->x;
			rect.y = ventana->y;
			rect.w = ventana->w;
			rect.h = ventana->h;
			
			SDL_BlitSurface (images[IMG_WINDOW], NULL, screen, &rect);
			
			/* dibujar las fichas antes del tablero */
			for (g = 0; g < 6; g++) {
				for (h = 0; h < 7; h++) {
					if (ventana->tablero[g][h] == 0) continue;
					rect.x = ventana->x + tablero_cols[h];
					rect.y = ventana->y + tablero_filas[g];
					
					rect.w = images[IMG_COINBLUE]->w;
					rect.h = images[IMG_COINBLUE]->h;
					
					if (ventana->tablero[g][h] == 1) {
						SDL_BlitSurface (images[IMG_COINRED], NULL, screen, &rect);
					} else {
						SDL_BlitSurface (images[IMG_COINBLUE], NULL, screen, &rect);
					}
				}
			}
			
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
				//printf ("Para hacer el resalte, turno: %i, Inicio: %i, Turno %% 2: %i\n", ventana->turno, ventana->inicio, (ventana->turno %2));
				if (ventana->turno % 2 == 0) { /* El que empieza siempre es rojo */
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

