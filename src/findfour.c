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
#include <SDL_mixer.h>
#include <SDL_ttf.h>

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
#include "inputbox.h"
#include "utf8.h"
#include "draw-text.h"
#include "message.h"

#define FPS (1000/24)

#define SWAP(a, b, t) ((t) = (a), (a) = (b), (b) = (t))

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE !FALSE
#endif

#ifdef WIN32
#	undef GAMEDATA_DIR
#	define GAMEDATA_DIR "./"
#endif

const char *images_names[NUM_IMAGES] = {
	GAMEDATA_DIR "images/lodge.png",
	GAMEDATA_DIR "images/lodge_fire.png",
	GAMEDATA_DIR "images/lodge_candle.png",
	
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
	
	GAMEDATA_DIR "images/loading.png",
	
	GAMEDATA_DIR "images/list-mini.png",
	GAMEDATA_DIR "images/list-big.png",
	
	GAMEDATA_DIR "images/win-1.png",
	GAMEDATA_DIR "images/win-2.png",
	GAMEDATA_DIR "images/win-3.png",
	GAMEDATA_DIR "images/win-4.png",
	
	GAMEDATA_DIR "images/window.png",
	GAMEDATA_DIR "images/tab.png",
	GAMEDATA_DIR "images/inputbox.png",
	
	GAMEDATA_DIR "images/chat.png",
	
	GAMEDATA_DIR "images/error.png",
	GAMEDATA_DIR "images/boton-error-up.png",
	GAMEDATA_DIR "images/boton-error-over.png",
	GAMEDATA_DIR "images/boton-error-down.png",
	
	GAMEDATA_DIR "images/boton-normal-up.png",
	GAMEDATA_DIR "images/boton-normal-over.png",
	GAMEDATA_DIR "images/boton-normal-down.png",
	
	GAMEDATA_DIR "images/fuzz.png"
};

const char *sound_names[NUM_SOUNDS] = {
	GAMEDATA_DIR "sounds/drop.wav",
	GAMEDATA_DIR "sounds/win.wav"
};

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
void drawfuzz (int x, int y, int w, int h);

/* Variables globales */
SDL_Surface * screen;
SDL_Surface * images [NUM_IMAGES];
SDL_Surface * text_waiting;
SDL_Surface * nick_image = NULL;
SDL_Surface * nick_image_blue = NULL;

int use_sound;
Mix_Chunk * sounds[NUM_SOUNDS];

static Ventana *primero, *ultimo, *drag;
static int drag_x, drag_y;

int server_port;
char nick_global[NICK_SIZE];
static int nick_default;

TTF_Font *ttf16_burbank_medium, *ttf14_facefront, *ttf16_comiccrazy;

Ventana *get_first_window (void) {
	return primero;
}

void set_first_window (Ventana *v) {
	primero = v;
}

void set_last_window (Ventana *v) {
	ultimo = v;
}

int analizador_hostname_puerto (const char *cadena, char *hostname, int *puerto) {
	char *p, *port, *invalid;
	int g;
	
	if (cadena[0] == 0) {
		hostname[0] = 0;
		return FALSE;
	}
	
	*puerto = 3300;
	
	if (cadena[0] == '[') {
		/* Es una ip literal, buscar por otro ] de cierre */
		p = strchr (cadena, ']');
		
		if (p == NULL) {
			/* Error, no hay cierre */
			hostname[0] = 0;
			return FALSE;
		}
		strncpy (hostname, &cadena[1], p - cadena - 1);
		hostname [p - cadena - 1] = 0;
		p++;
		cadena = p;
	} else {
		/* Nombre de host directo */
		port = strchr (cadena, ':');
		
		if (port == NULL) {
			/* No hay puerto, sólo host directo */
			strcpy (hostname, cadena);
			
			return TRUE;
		} else {
			/* Copiar hasta antes del ":" */
			strncpy (hostname, cadena, port - cadena);
			hostname [port - cadena] = 0;
		}
		cadena = port;
	}
	
	/* Buscar por un posible ":" */
	if (cadena[0] == ':') {
		g = strtol (&cadena[1], &invalid, 10);
		
		if (invalid[0] != 0 || g > 65535) {
			/* Caracteres sobrantes */
			hostname[0] = 0;
			return FALSE;
		}
		
		*puerto = g;
	}
	
	return TRUE;
}

void render_nick (void) {
	SDL_Color blanco, negro;
	
	if (nick_image != NULL) SDL_FreeSurface (nick_image);
	if (nick_image_blue != NULL) SDL_FreeSurface (nick_image_blue);
	
	negro.r = negro.g = negro.b = 0;
	blanco.r = blanco.g = blanco.b = 255;
	nick_image = draw_text_with_shadow (ttf16_comiccrazy, 2, nick_global, blanco, negro);
	
	blanco.r = 0xD5; blanco.g = 0xF1; blanco.b = 0xff;
	negro.r = 0x33; negro.g = 0x66; negro.b = 0x99;
	nick_image_blue = draw_text_with_shadow (ttf16_comiccrazy, 2, nick_global, blanco, negro);
}

void change_nick (InputBox *ib, const char *texto) {
	int len, g;
	int chars;
	if (strcmp (texto, "") != 0) {
		len = strlen (texto);
		
		if (len > 15) {
			chars = 0;
			while (chars < u8_strlen ((char *)texto)) {
				if (u8_offset ((char *) texto, chars + 1) < 16) {
					chars++;
				} else {
					break;
				}
			}
			
			len = u8_offset ((char *)texto, chars);
		}
		
		/* Copiar el texto a la variable de nick */
		strncpy (nick_global, texto, len + 1);
		for (g = len; g < 16; g++) {
			nick_global[g] = 0;
		}
		
		render_nick ();
		/* Reenviar el broadcast */
		enviar_broadcast_game (nick_global);
	}
	
	/* Eliminar esta ventana de texto */
	eliminar_inputbox (ib);
}

void nueva_conexion (InputBox *ib, const char *texto) {
	int valido, puerto;
	char *hostname;
	Ventana *ventana;
	
	hostname = strdup (texto);
	
	valido = analizador_hostname_puerto (texto, hostname, &puerto);
	
	if (valido) {
		/* Pasar a intentar hacer una conexión */
		ventana = (Ventana *) crear_juego ();
	
		conectar_con ((Juego *) ventana, nick_global, hostname, puerto);
	} else {
		/* Mandar un mensaje de error */
	}
	
	free (hostname);
	
	/* Eliminar esta ventana de texto */
	eliminar_inputbox (ib);
}

int main (int argc, char *argv[]) {
	int r;
	
	memset (nick_global, 0, sizeof (nick_global));
	
	setup ();
	
	/* Generar o leer el nick del archivo de configuración */
	r = RANDOM (65536);
	sprintf (nick_global, "Player%i", r);
	render_nick ();
	
	nick_default = 1;
	
	server_port = 3300;
	
	if (argc > 1) {
		server_port = atoi (argv[1]);
	}
	
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
	SDL_Rect rect, rect2;
	int *map = NULL;
	
	int g, h;
	int start = 0;
	int x, y;
	int manejado;
	int background_fire;
	int lodge_candle;
	
	primero = NULL;
	ultimo = NULL;
	Ventana *ventana, *next;
	
	//SDL_EventState (SDL_MOUSEMOTION, SDL_IGNORE);
	
	drag = NULL;
	
	if (findfour_netinit (server_port) < 0) {
		fprintf (stderr, "Falló al inicializar la red\n");
		
		return GAME_QUIT;
	}
	
	inicializar_chat ();
	
	if (nick_default) {
		crear_inputbox ((InputBoxFunc) change_nick, "Ingrese su nombre de jugador:", nick_global);
	}
	
	SDL_EnableUNICODE (1);
	background_fire = 0;
	lodge_candle = 0;
	
	do {
		last_time = SDL_GetTicks ();
		
		/* Antes de procesar los eventos locales, procesar la red */
		process_netevent ();
		
		while (SDL_PollEvent(&event) > 0) {
			switch (event.type) {
				case SDL_KEYDOWN:
					/* Encontrar la primer ventana no oculta para enviarle el evento */
					manejado = FALSE;
					ventana = primero;
					
					while (ventana != NULL && ventana->mostrar != TRUE) ventana = ventana->next;
					
					if (list_msg != NULL) ventana = NULL; /* No hay eventos de teclado cuando hay un mensaje en pantalla */
					
					if (ventana != NULL) {
						/* Revisar si le interesa nuestro evento */
						if (ventana->key_down != NULL) {
							manejado = ventana->key_down (ventana, &event.key);
						}
					}
					
					/* Si el evento aún no ha sido manejado por alguna ventana, es de nuestro interés */
					if (!manejado) {
						if (event.key.keysym.sym == SDLK_F5) {
							ventana = (Ventana *) crear_inputbox ((InputBoxFunc) nueva_conexion, "Dirección a conectar:", "");
						} else if (event.key.keysym.sym == SDLK_F8) {
							show_chat ();
						}
					}
					break;
				case SDL_MOUSEBUTTONDOWN:
					/* Primero, analizar si el evento cae dentro de alguna ventana */
					
					if (event.button.button != SDL_BUTTON_LEFT) break;
					map = NULL;
					manejado = FALSE;
					if (list_msg != NULL) {
						message_mouse_down (event.button.x, event.button.y, &map);
						manejado = TRUE;
					}
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
						
						if (list_msg != NULL) {
							message_mouse_motion (event.motion.x, event.motion.y, &map);
							manejado = TRUE;
						}
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
								int *t;
								ventana->mouse_motion (ventana, -1, -1, &t);
							}
							
							ventana = ventana->next;
						}
						if (!manejado) {
							/* Si el movimiento del mouse no entró a alguna ventana, es para el fondo */
							if (x >= 253 && x < 272 && y >= 117 && y < 179 && lodge_candle < 14) {
								lodge_candle = 14;
							}
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
					
					if (list_msg != NULL) {
						message_mouse_up (event.button.x, event.button.y, &map);
						manejado = TRUE;
					}
					
					ventana = primero;
					while (ventana != NULL && manejado == FALSE) {
						next = ventana->next;
						/* Si el evento hace match por las coordenadas, preguntarle a la ventana si lo quiere manejar */
						if (x >= ventana->x && x < ventana->x + ventana->w && y >= ventana->y && y < ventana->y + ventana->h) {
							if (ventana->mouse_up != NULL) {
								manejado = ventana->mouse_up (ventana, x - ventana->x, y - ventana->y, &map);
							}
						}
						ventana = next;
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
		
		/* La fogata del lodge */
		g = background_fire / 2;
		if (g > 0) {
			rect2.x = (g - 1) * 47;
			rect2.y = 0;
			rect.w = rect2.w = 47;
			rect.h = rect2.h = 42;
			
			rect.x = 638;
			rect.y = 217;
			SDL_BlitSurface (images[IMG_LODGE_FIRE], &rect2, screen, &rect);
		}
		background_fire++;
		if (background_fire >= 10) background_fire = 0;
		
		/* La vela del lodge */
		if (lodge_candle < 30) {
			g = lodge_candle / 2;
		} else if (lodge_candle >= 30 && lodge_candle < 38) {
			g = 15;
		} else {
			g = 16 + (lodge_candle - 38) / 2;
		}
		
		if (g > 0) {
			rect2.x = (g - 1) * 15;
			rect2.y = 0;
			rect2.w = rect.w = 15;
			rect2.h = rect.h = 42;
			
			rect.x = 254;
			rect.y = 95;
			SDL_BlitSurface (images[IMG_LODGE_CANDLE], &rect2, screen, &rect);
		}
		
		if (lodge_candle == 13) {
			lodge_candle = 0;
		} else if (lodge_candle == 41) {
			lodge_candle = 4;
		} else {
			lodge_candle++;
		}
		
		//printf ("Dibujar: \n");
		for (ventana = ultimo; ventana != NULL; ventana = ventana->prev) {
			if (!ventana->mostrar) continue;
			
			ventana->draw (ventana, screen);
		}
		
		/* Dibujar los mensajes en pantalla */
		if (list_msg != NULL) {
			drawfuzz (0, 0, 760, 480);
			message_display (screen);
		}
		
		SDL_Flip (screen);
		
		now_time = SDL_GetTicks ();
		if (now_time < last_time + FPS) SDL_Delay(last_time + FPS - now_time);
		
	} while (!done);
	SDL_EnableUNICODE (0);
	
	/* FIXME: Recorrer todas las ventanas y enviar un fin a todas las ventanas de juego */
	
	findfour_netclose ();
	
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

void full_stop_drag (void) {
	drag = NULL;
}

void drawfuzz (int x, int y, int w, int h) {
	int xx, yy;
	SDL_Rect src, dest;

	for (yy = y; yy < y + h; yy = yy + (images[IMG_FUZZ] -> h)) {
		for (xx = x; xx < x + w; xx = xx + (images[IMG_FUZZ] -> w)) {
			src.x = 0;
			src.y = 0;
			src.w = images[IMG_FUZZ] -> w;
			src.h = images[IMG_FUZZ] -> h;

			if (xx + src.w > x + w)
			src.w = x + w - xx;

			if (yy + src.h > y + h)
			src.h = y + h - yy;

			dest.x = xx;
			dest.y = yy;

			dest.w = src.w;
			dest.h = src.h;

			SDL_BlitSurface(images[IMG_FUZZ], &src, screen, &dest);
		}
	}
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
	TTF_Font *font;
	SDL_Color blanco;
	int g;
	
	/* Inicializar el Video SDL */
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		fprintf (stderr,
			"Error: Can't initialize the video subsystem\n"
			"The error returned by SDL is:\n"
			"%s\n", SDL_GetError());
		exit (1);
	}
	
	image = IMG_Load (GAMEDATA_DIR "images/icon.png");
	if (image) {
		SDL_WM_SetIcon (image, NULL);
		SDL_FreeSurface (image);
	}
	SDL_WM_SetCaption ("Find Four", "Find Four");
	
	/* Crear la pantalla de dibujado */
	screen = set_video_mode (0);
	
	if (screen == NULL) {
		fprintf (stderr,
			"Error: Can't setup 760x480 video mode.\n"
			"The error returned by SDL is:\n"
			"%s\n", SDL_GetError());
		exit (1);
	}
	
	use_sound = 1;
	if (SDL_InitSubSystem (SDL_INIT_AUDIO) < 0) {
		fprintf (stdout,
			"Warning: Can't initialize the audio subsystem\n"
			"Continuing...\n");
		use_sound = 0;
	}
	
	if (use_sound) {
		/* Inicializar el sonido */
		if (Mix_OpenAudio (22050, AUDIO_S16, 2, 4096) < 0) {
			fprintf (stdout,
				"Warning: Can't initialize the SDL Mixer library\n");
			use_sound = 0;
		} else {
			Mix_AllocateChannels (3);
		}
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
	
	if (use_sound) {
		for (g = 0; g < NUM_SOUNDS; g++) {
			sounds[g] = Mix_LoadWAV (sound_names[g]);
			
			if (sounds[g] == NULL) {
				fprintf (stderr,
					"Failed to load data file:\n"
					"%s\n"
					"The error returned by SDL is:\n"
					"%s\n", sound_names[g], SDL_GetError ());
				SDL_Quit ();
				exit (1);
			}
			Mix_VolumeChunk (sounds[g], MIX_MAX_VOLUME / 2);
		}
	}
	
	/* Cargar las tipografias */
	if (TTF_Init () < 0) {
		fprintf (stderr,
			"Error: Can't initialize the SDL TTF library\n"
			"%s\n", TTF_GetError ());
		SDL_Quit ();
		exit (1);
	}
	
	ttf16_burbank_medium = TTF_OpenFont (GAMEDATA_DIR "burbankbm.ttf", 16);
	
	if (!ttf16_burbank_medium) {
		fprintf (stderr,
			"Failed to load font file 'Burbank Medium Bold'\n"
			"The error returned by SDL is:\n"
			"%s\n", TTF_GetError ());
		SDL_Quit ();
		exit (1);
	}
	
	ttf14_facefront = TTF_OpenFont (GAMEDATA_DIR "ccfacefront.ttf", 14);
	if (!ttf14_facefront) {
		fprintf (stderr,
			"Failed to load font file 'CC Face Front'\n"
			"The error returned by SDL is:\n"
			"%s\n", TTF_GetError ());
		SDL_Quit ();
		exit (1);
	}
	
	ttf16_comiccrazy = TTF_OpenFont (GAMEDATA_DIR "comicrazy.ttf", 16);
	if (!ttf16_comiccrazy) {
		fprintf (stderr,
			"Failed to load font file 'Comic Crazy'\n"
			"The error returned by SDL is:\n"
			"%s\n", TTF_GetError ());
		SDL_Quit ();
		exit (1);
	}
	
	font = TTF_OpenFont (GAMEDATA_DIR "burbankbm.ttf", 12);
	if (!font) {
		fprintf (stderr,
			"Failed to load font file 'Burbank Medium Bold'\n"
			"The error returned by SDL is:\n"
			"%s\n", TTF_GetError ());
		SDL_Quit ();
		exit (1);
	}
	
	blanco.r = blanco.g = blanco.b = 255;
	text_waiting = TTF_RenderUTF8_Blended (font, "Waiting for player", blanco);
	
	TTF_CloseFont (font);
	srand (SDL_GetTicks ());
}

