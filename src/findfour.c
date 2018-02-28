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

#include <locale.h>
#include "gettext.h"
#define _(string) gettext (string)

#include "path.h"

#include "ventana.h"
#include "findfour.h"
#include "background.h"
#include "juego.h"
#include "netplay.h"
#include "chat.h"
#include "inputbox.h"
#include "utf8.h"
#include "draw-text.h"
#include "message.h"
#include "resolv.h"

#define FPS (1000/24)

#define SWAP(a, b, t) ((t) = (a), (a) = (b), (b) = (t))

const char *images_names[NUM_IMAGES] = {
	"images/ventana.png",
	"images/tablero.png",
	"images/coinblue.png",
	"images/coinred.png",
	
	"images/bigcoinblue.png",
	"images/bigcoinred.png",
	
	"images/ventana-chat.png",
	
	"images/shadow-up.png",
	"images/shadow-over.png",
	"images/shadow-down.png",
	
	"images/button-up.png",
	"images/button-over.png",
	"images/button-down.png",
	
	"images/button-arrow-1-up.png",
	"images/button-arrow-1-over.png",
	"images/button-arrow-1-down.png",
	
	"images/button-arrow-2-up.png",
	"images/button-arrow-2-over.png",
	"images/button-arrow-2-down.png",
	
	"images/button-list-up.png",
	"images/button-list-over.png",
	"images/button-list-down.png",
	
	"images/loading.png",
	
	"images/list-mini.png",
	"images/recent-mini.png",
	"images/list-big.png",
	
	"images/win-1.png",
	"images/win-2.png",
	"images/win-3.png",
	"images/win-4.png",
	
	"images/window.png",
	"images/window_shadow.png",
	"images/tab.png",
	"images/inputbox.png",
	
	"images/chat.png",
	
	"images/error.png",
	"images/error_shadow.png",
	"images/boton-error-up.png",
	"images/boton-error-over.png",
	"images/boton-error-down.png",
	
	"images/boton-normal-up.png",
	"images/boton-normal-over.png",
	"images/boton-normal-down.png",
	
	"images/fuzz.png"
};

const char *sound_names[NUM_SOUNDS] = {
	"sounds/drop.wav",
	"sounds/win.wav"
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

static InputBox *connect_window;
static InputBox *change_nick_window;

int server_port;
char nick_global[NICK_SIZE];
static int nick_default;

TTF_Font *ttf16_burbank_medium, *ttf14_facefront, *ttf16_comiccrazy, *ttf20_comiccrazy;

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
	
	/* Programar el cambio de nick en todas las ventanas de juego */
	update_local_nick ();
	
	/* Eliminar esta ventana de texto */
	eliminar_inputbox (ib);
	change_nick_window = NULL;
}

void late_connect (const char *hostname, int port, const struct addrinfo *res, int error, const char *errstr) {
	if (res == NULL) {
		message_add (MSG_ERROR, _("OK"), _("Error resolving the hostname '%s':\n%s"), hostname, errstr);
		return;
	}
	
	Ventana *ventana;
	
	ventana = (Ventana *) crear_juego (TRUE);
		
	/* Agregar la conexión a las partidas recientes */
	conectar_con_sockaddr ((Juego *) ventana, nick_global, res->ai_addr, res->ai_addrlen);
}

void nueva_conexion (InputBox *ib, const char *texto) {
	int valido, puerto;
	char *hostname;
	Ventana *ventana;
	
	if (strcmp (texto, "") == 0) {
		/* Texto vacio, ignorar */
		if (ib != NULL) {
			/* Eliminar esta ventana de texto */
			eliminar_inputbox (ib);
			connect_window = NULL;
		}
		return;
	}
	hostname = strdup (texto);
	
	valido = analizador_hostname_puerto (texto, hostname, &puerto, 3300);
	
	if (valido) {
		/* Ejecutar la resolución de nombres primero, conectar después */
		do_query (hostname, puerto, late_connect);
		
		buddy_list_recent_add (texto);
	} else {
		/* Mandar un mensaje de error */
	}
	
	free (hostname);
	
	if (ib != NULL) {
		/* Eliminar esta ventana de texto */
		eliminar_inputbox (ib);
		connect_window = NULL;
	}
}

void cancelar_conexion (InputBox *ib, const char *texto) {
	connect_window = NULL;
}

void cancelar_change_nick (InputBox *ib, const char *texto) {
	change_nick_window = NULL;
}

int findfour_default_keyboard_handler (Ventana *v, SDL_KeyboardEvent *key) {
	if (key->keysym.sym == SDLK_F5) {
		if (connect_window != NULL) {
			/* Levantar la ventana de conexión al frente */
			window_raise (connect_window->ventana);
		} else {
			/* Crear un input box para conectar */
			connect_window = crear_inputbox ((InputBoxFunc) nueva_conexion, _("Hostname or direction to connect:"), "", cancelar_conexion);
		}
	} else if (key->keysym.sym == SDLK_F8) {
		show_chat ();
	} else if (key->keysym.sym == SDLK_F7) {
		if (change_nick_window != NULL) {
			window_raise (change_nick_window->ventana);
		} else {
			change_nick_window = crear_inputbox ((InputBoxFunc) change_nick, _("Please choose a nickname:"), nick_global, cancelar_change_nick);
		}
	}
	
	return TRUE;
}

int main (int argc, char *argv[]) {
	int r;

	init_resolver ();
	
	initSystemPaths (argv[0]);
	
	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, l10n_path);
	
	textdomain (PACKAGE);
	
	memset (nick_global, 0, sizeof (nick_global));
	
	setup ();
	
	/* Generar o leer el nick del archivo de configuración */
	r = RANDOM (65536);
	sprintf (nick_global, _("Player %i"), r);
	render_nick ();
	
	nick_default = 1;
	
	server_port = 3300;
	
	do {
		if (game_loop () == GAME_QUIT) break;
	} while (1 == 0);
	
	destroy_resolver ();
	
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
	
	Ventana *ventana, *next;
	Juego *j;
	
	//SDL_EventState (SDL_MOUSEMOTION, SDL_IGNORE);
	
	if (findfour_netinit (server_port) < 0) {
		fprintf (stderr, _("Failed to init network\n"));
		
		return GAME_QUIT;
	}
	
	window_register_default_keyboard_events (findfour_default_keyboard_handler, NULL);
	
	draw_background (screen, NULL);
	inicializar_chat ();
	
	if (nick_default) {
		change_nick_window = crear_inputbox ((InputBoxFunc) change_nick, _("Please choose a nickname:"), nick_global, cancelar_change_nick);
	}
	
	SDL_Flip (screen);
	
	SDL_EnableUNICODE (1);
	
	do {
		last_time = SDL_GetTicks ();
		
		/* Antes de procesar los eventos locales, procesar la red */
		process_netevent ();
		
		while (SDL_PollEvent(&event) > 0) {
			if (event.type == SDL_QUIT) {
				/* Vamos a cerrar la aplicación */
				done = GAME_QUIT;
				break;
			} else if (event.type == SDL_MOUSEBUTTONDOWN ||
			           event.type == SDL_MOUSEMOTION ||
			           event.type == SDL_KEYDOWN ||
			           event.type == SDL_MOUSEBUTTONUP) {
				window_manager_event (event);
			}
		}
		
		window_manager_timer ();
		
		//printf ("Dibujar: \n");
		window_manager_draw (screen);
		
		now_time = SDL_GetTicks ();
		if (now_time < last_time + FPS) SDL_Delay(last_time + FPS - now_time);
		
	} while (!done);
	SDL_EnableUNICODE (0);
	
	/* Recorrer los juegos */
	j = get_game_list ();
	while (j != NULL) {
		if (j->estado != NET_CLOSED && j->estado != NET_WAIT_CLOSING) {
			j->last_fin = NET_USER_QUIT;
			j->retry = 0;
			enviar_fin (j);
		}
		
		j = j->next;
	}
	
	findfour_netclose ();
	
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
	TTF_Font *font;
	SDL_Color blanco, negro;
	int g;
	char buffer_file[8192];
	
	/* Inicializar el Video SDL */
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		fprintf (stderr,
			_("Error: Can't initialize the video subsystem\n"
			"The error returned by SDL is:\n"
			"%s\n"), SDL_GetError());
		exit (1);
	}
	
	sprintf (buffer_file, "%simages/icon.png", systemdata_path);
	image = IMG_Load (buffer_file);
	if (image) {
		SDL_WM_SetIcon (image, NULL);
		SDL_FreeSurface (image);
	}
	SDL_WM_SetCaption (_("Find Four"), _("Find Four"));
	
	/* Crear la pantalla de dibujado */
	screen = set_video_mode (0);
	
	if (screen == NULL) {
		fprintf (stderr,
			_("Error: Can't setup 760x480 video mode.\n"
			"The error returned by SDL is:\n"
			"%s\n"), SDL_GetError());
		exit (1);
	}
	
	use_sound = 1;
	if (SDL_InitSubSystem (SDL_INIT_AUDIO) < 0) {
		fprintf (stdout,
			_("Warning: Can't initialize the audio subsystem\n"
			"Continuing...\n"));
		use_sound = 0;
	}
	
	if (use_sound) {
		/* Inicializar el sonido */
		if (Mix_OpenAudio (22050, AUDIO_S16, 2, 4096) < 0) {
			fprintf (stdout,
				_("Warning: Can't initialize the SDL Mixer library\n"));
			use_sound = 0;
		} else {
			Mix_AllocateChannels (3);
		}
	}
	
	for (g = 0; g < NUM_IMAGES; g++) {
		sprintf (buffer_file, "%s%s", systemdata_path, images_names[g]);
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
		
		images[g] = image;
		/* TODO: Mostrar la carga de porcentaje */
	}
	
	if (use_sound) {
		for (g = 0; g < NUM_SOUNDS; g++) {
			sprintf (buffer_file, "%s%s", systemdata_path, sound_names[g]);
			sounds[g] = Mix_LoadWAV (buffer_file);
			
			if (sounds[g] == NULL) {
				fprintf (stderr,
					_("Failed to load data file:\n"
					"%s\n"
					"The error returned by SDL is:\n"
					"%s\n"), buffer_file, SDL_GetError ());
				SDL_Quit ();
				exit (1);
			}
			Mix_VolumeChunk (sounds[g], MIX_MAX_VOLUME / 2);
		}
	}
	
	/* Cargar las tipografias */
	if (TTF_Init () < 0) {
		fprintf (stderr,
			_("Error: Can't initialize the SDL TTF library\n"
			"%s\n"), TTF_GetError ());
		SDL_Quit ();
		exit (1);
	}
	
	sprintf (buffer_file, "%s%s", systemdata_path, "burbankbm.ttf");
	ttf16_burbank_medium = TTF_OpenFont (buffer_file, 16);
	
	if (!ttf16_burbank_medium) {
		fprintf (stderr,
			_("Failed to load font file 'Burbank Medium Bold'\n"
			"The error returned by SDL is:\n"
			"%s\n"), TTF_GetError ());
		SDL_Quit ();
		exit (1);
	}
	
	sprintf (buffer_file, "%s%s", systemdata_path, "ccfacefront.ttf");
	ttf14_facefront = TTF_OpenFont (buffer_file, 14);
	if (!ttf14_facefront) {
		fprintf (stderr,
			_("Failed to load font file 'CC Face Front'\n"
			"The error returned by SDL is:\n"
			"%s\n"), TTF_GetError ());
		SDL_Quit ();
		exit (1);
	}
	
	SDL_RWops *ttf_comiccrazy;
	sprintf (buffer_file, "%s%s", systemdata_path, "comicrazy.ttf");
	ttf_comiccrazy = SDL_RWFromFile (buffer_file, "rb");
	
	if (ttf_comiccrazy == NULL) {
		fprintf (stderr,
			_("Failed to load font file 'Comic Crazy'\n"
			"The error returned by SDL is:\n"
			"%s\n"), TTF_GetError ());
		SDL_Quit ();
		exit (1);
	}
	
	SDL_RWseek (ttf_comiccrazy, 0, RW_SEEK_SET);
	ttf16_comiccrazy = TTF_OpenFontRW (ttf_comiccrazy, 0, 16);
	
	SDL_RWseek (ttf_comiccrazy, 1, RW_SEEK_SET);
	ttf20_comiccrazy = TTF_OpenFontRW (ttf_comiccrazy, 0, 20);
	
	if (!ttf16_comiccrazy || !ttf20_comiccrazy) {
		SDL_Quit ();
		exit (1);
	}
	
	bind_textdomain_codeset (PACKAGE, "UTF-8");
	
	/* Dibujar el texto de "Esperando jugador" */
	blanco.r = 0xD5; blanco.g = 0xF1; blanco.b = 0xff;
	negro.r = 0x33; negro.g = 0x66; negro.b = 0x99;
	text_waiting = draw_text_with_shadow (ttf16_comiccrazy, 2, _("Connecting..."), blanco, negro);
	
	setup_background ();
	srand (SDL_GetTicks ());
}

