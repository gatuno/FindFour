/*
 * juego.c
 * This file is part of Find Four
 *
 * Copyright (C) 2015 - Félix Arreola Rodríguez
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gettext.h"
#define _(string) gettext (string)

/* Para el manejo de la red */
#ifdef __MINGW32__
#	include <winsock2.h>
#	include <ws2tcpip.h>
#else
#	include <sys/socket.h>
#	include <netinet/in.h>
#endif
#include <sys/types.h>

#include "findfour.h"
#include "juego.h"
#include "netplay.h"
#include "draw-text.h"
#include "message.h"
#include "ventana.h"

#define ANIM_VEL 2

enum {
	JUEGO_BUTTON_CLOSE = 0,
	
	JUEGO_NUM_BUTTONS
};

int juego_mouse_down (Ventana *v, int x, int y);
int juego_mouse_motion (Ventana *v, int x, int y);
int juego_mouse_up (Ventana *v, int x, int y);
void juego_draw_button_close (Ventana *v, int frame);
void juego_button_frame (Ventana *, int button, int frame);
void juego_button_event (Ventana *, int button);

void juego_dibujar_resalte (Juego *j);
void juego_dibujar_tablero (Juego *j);

/* Algunas constantes */
static const int tablero_cols[7] = {32, 56, 81, 105, 129, 153, 178};
static const int tablero_filas[6] = {69, 93, 117, 141, 166, 190};
static const int resalte_pos[7] = {30, 53, 79, 102, 126, 150, 175};

static Juego *network_game_list = NULL;

Juego *get_game_list (void) {
	return network_game_list;
}

void update_local_nick (void) {
	Juego *j;
	
	j = network_game_list;
	while (j != NULL) {
		/* Dejar el envio del nick hasta que se envie un movimiento */
		j->resend_nick = 1;
		juego_dibujar_tablero (j);
		j = j->next;
	}
}

void juego_first_time_draw (Juego  *j) {
	SDL_Surface *surface;
	SDL_Rect rect;
	
	surface = window_get_surface (j->ventana);
	
	SDL_FillRect (surface, NULL, 0); /* Transparencia total */
	
	SDL_SetAlpha (images[IMG_WINDOW], 0, 0);
	
	SDL_BlitSurface (images[IMG_WINDOW], NULL, surface, NULL);
	
	/* Dibujar el nombre del jugador local */
	rect.x = 74;
	rect.y = 242;
	rect.w = nick_image->w;
	rect.h = nick_image->h;
	
	SDL_BlitSurface (nick_image_blue, NULL, surface, &rect);
	
	/* Dibujar el texto de "conectando" */
	rect.x = 74;
	rect.y = 272;
	rect.w = text_waiting->w;
	rect.h = text_waiting->h;
	
	SDL_BlitSurface (text_waiting, NULL, surface, &rect);
	
	/* Dibujar el botón de cierre */
	juego_draw_button_close (j->ventana, 0);
	
	/* Dibujar el tablero */
	rect.x = 26;
	rect.y = 65;
	rect.w = images[IMG_BOARD]->w;
	rect.h = images[IMG_BOARD]->h;
	
	SDL_BlitSurface (images[IMG_BOARD], NULL, surface, &rect);
	
	window_flip (j->ventana);
}

int juego_timer_callback_cargando (Ventana *v) {
	Juego *j;
	SDL_Rect rect, rect2;
	SDL_Surface *surface;
	
	surface = window_get_surface (v);
	
	j = (Juego *) window_get_data (v);
	
	/* Borrar con la ventana */
	rect.x = 39;
	rect.y = 235;
	rect.h = 64;
	rect.w = 32;
	
	window_update (v, &rect);
	
	SDL_BlitSurface (images[IMG_WINDOW], &rect, surface, &rect);
	
	/* Los botones de carga */
	if (j->estado == NET_SYN_SENT) {
		rect.x = 39;
		rect.y = 235;
		rect.w = 32;
		rect.h = 32;
	
		rect2.x = (j->timer % 19) * 32;
		rect2.y = 0;
		rect2.w = rect2.h = 32;
	
		SDL_BlitSurface (images[IMG_LOADING], &rect2, surface, &rect);
		
		rect.x = 39;
		rect.y = 265;
		rect.w = 32;
		rect.h = 32;
	
		rect2.x = (j->timer % 19) * 32;
		rect2.y = 0;
		rect2.w = rect2.h = 32;
		
		SDL_BlitSurface (images[IMG_LOADING], &rect2, surface, &rect);
	}
	
	j->timer++;
	if (j->timer >= 19) j->timer = 0;
	
	return TRUE;
}

int juego_timer_callback_anim (Ventana *v) {
	int g, h, i;
	Juego *j;
	SDL_Surface *surface;
	SDL_Rect rect;
	
	j = (Juego *) window_get_data (v);
	surface = window_get_surface (v);
	
	if (j->num_a > 0) {
		j->animaciones[0].frame++;
		g = j->animaciones[0].frame / ANIM_VEL;
		if (g == j->animaciones[0].fila) {
			/* Finalizar esta animación */
			j->tablero[j->animaciones[0].fila][j->animaciones[0].col] = j->animaciones[0].color;
			
			/* Recorrer las otras animaciones */
			for (h = 1; h < j->num_a; h++) {
				j->animaciones[h - 1] = j->animaciones[h];
			}
			j->num_a--;
		}
	}
	
	juego_dibujar_tablero (j);
	
	if (j->num_a == 0) {
		if (j->win != 0) {
			i = IMG_WIN_1 + (j->win_dir - 1);
			rect.w = images[i]->w;
			rect.h = images[i]->h;
			rect.x = 28 + (j->win_col * 24);
			rect.y = 65 + ((5 - j->win_fila) * 24);
			
			if (j->win_dir == 2) {
				rect.y = 65 + ((5 - j->win_fila - 3) * 24);
			}
			
			SDL_BlitSurface (images[i],  NULL, surface, &rect);
		}
		
		if (j->win != 0 && (j->win - 1) == j->inicio) {
			// Somos ganadores
			message_add (MSG_NORMAL, _("Ok"), _("You won"));
		} else if (j->turno == 42) {
			message_add (MSG_NORMAL, _("Ok"), _("%s and you have tied"), j->nick_remoto);
		}
		return FALSE;
	}
	
	return TRUE;
}

Juego *crear_juego (int top_window) {
	Juego *j, *lista;
	int correct;
	
	/* Crear un nuevo objeto Juego */
	j = (Juego *) malloc (sizeof (Juego));
	
	/* Crear una ventana */
	j->ventana = window_create (232, 324, top_window);
	window_set_data (j->ventana, j);
	
	window_register_mouse_events (j->ventana, juego_mouse_down, juego_mouse_motion, juego_mouse_up);
	window_register_buttons (j->ventana, JUEGO_NUM_BUTTONS, juego_button_frame, juego_button_event);
	
	/* Valores propios del juego */
	j->turno = 0;
	j->win = 0;
	j->inicio = -1;
	j->resalte = -1;
	j->timer = 0;
	
	j->local = j->remote = 0;
	j->retry = 0;
	j->estado = NET_CLOSED;
	j->close_frame = 0;
	
	/* Para la animación */
	j->num_a = 0;
	
	/* Para resincronizar el nick */
	j->resend_nick = 0;
	j->wait_nick_ack = 0;
	
	/* Vaciar el tablero */
	memset (j->tablero, 0, sizeof (int[6][7]));
	
	/* Vaciar el nick del otro jugador */
	memset (j->nick_remoto, 0, sizeof (j->nick_remoto));
	j->nick_remoto_image = NULL;
	j->nick_remoto_image_blue = NULL;
	
	/* Generar un número local aleatorio */
	do {
		correct = 0;
		j->local = 1 + RANDOM (65534);
		
		lista = network_game_list;
		while (lista != NULL) {
			if (lista->local == j->local) correct = 1;
			lista = lista->next;
		}
	} while (correct);
	
	/* Ligar el nuevo objeto juego */
	j->next = network_game_list;
	network_game_list = j;
	
	/* Dibujar la ventana la primera vez */
	
	window_register_timer_events (j->ventana, juego_timer_callback_cargando);
	window_enable_timer (j->ventana);
	juego_first_time_draw  (j);
	
	return j;
}

void eliminar_juego (Juego *j) {
	/* Desligar completamente */
	if (j->ventana != NULL) {
		/* Destruir la ventana asociada con este juego */
		window_destroy (j->ventana);
		j->ventana = NULL;
	}
	
	if (j->nick_remoto_image != NULL) SDL_FreeSurface (j->nick_remoto_image);
	if (j->nick_remoto_image_blue != NULL) SDL_FreeSurface (j->nick_remoto_image_blue);
	
	/* Sacar de la lista ligada simple */
	if (network_game_list == j) {
		/* Primero */
		network_game_list = j->next;
	} else {
		Juego *prev;
		
		prev = network_game_list;
		while (prev->next != j) prev = prev->next;
		
		prev->next = j->next;
	}
	
	free (j);
}

void juego_button_frame (Ventana *v, int button, int frame) {
	Juego *j;
	if (button == JUEGO_BUTTON_CLOSE) {
		/* Redibujar el botón */
		juego_draw_button_close (v, frame);
		j = (Juego *) window_get_data (v);
		j->close_frame = frame;
	}
}

void juego_button_event (Ventana *v, int button) {
	Juego *j;
	if (button == JUEGO_BUTTON_CLOSE) {
		/* Quitar esta ventana */
		j = (Juego *) window_get_data (v);
		if (j->estado != NET_CLOSED) {
			j->last_fin = NET_USER_QUIT;
			j->retry = 0;
			/* Destruir la ventana asociada con este juego */
			window_destroy (j->ventana);
			j->ventana = NULL;
			
			enviar_fin (j);
		} else {
			eliminar_juego (j);
		}
	}
}

int juego_mouse_down (Ventana *v, int x, int y) {
	if (x >= 64 && x < 168 && y < 22) {
		/* Click por el agarre */
		window_start_drag (v, x, y);
		return TRUE;
	} else if (y >= 26 && y < 54 && x >= 192 && x < 220) {
		/* El click cae en el botón de cierre de la ventana */
		/* FIXME: Arreglar lo de los botones */
		window_button_mouse_down (v, JUEGO_BUTTON_CLOSE);
		return TRUE;
	} else if (y >= 16) {
		/* El evento cae dentro de la ventana */
		return TRUE;
	}
	
	return FALSE;
}

void juego_dibujar_resalte (Juego *j) {
	Uint32 color;
	SDL_Surface *surface;
	SDL_Rect rect, rect2;
	int g;
	
	surface = window_get_surface (j->ventana);
	
	color = SDL_MapRGB (surface->format, 0x00, 0x52, 0x9b);
	
	rect.x = 26;
	rect.y = 46;
	rect.w = 182;
	rect.h = 47;
	
	SDL_FillRect (surface, &rect, color);
	window_update (j->ventana, &rect);
	
	/* Redibujar las fichas de la fila superior */
	for (g = 0; g < 7; g++) {
		if (j->tablero[0][g] == 0) continue;
		rect.x = tablero_cols[g];
		rect.y = tablero_filas[0];
		
		rect.w = images[IMG_COINBLUE]->w;
		rect.h = images[IMG_COINBLUE]->h;
		
		if (j->tablero[0][g] == 1) {
			SDL_BlitSurface (images[IMG_COINRED], NULL, surface, &rect);
		} else {
			SDL_BlitSurface (images[IMG_COINBLUE], NULL, surface, &rect);
		}
	}
	
	/* Si existe alguna pieza en animación sobre la fila superior, redibujar */
	if (j->num_a > 0) {
		g = j->animaciones[0].frame / ANIM_VEL;
		if (g == 0) {
			rect.x = tablero_cols[j->animaciones[0].col];
			rect.y = tablero_filas[0];
			rect.w = images[IMG_COINBLUE]->w;
			rect.h = images[IMG_COINBLUE]->h;
		
			if (j->animaciones[0].color == 1) {
				SDL_BlitSurface (images[IMG_COINRED], NULL, surface, &rect);
			} else {
				SDL_BlitSurface (images[IMG_COINBLUE], NULL, surface, &rect);
			}
		}
	}
	
	/* Redibujar el pedazo de tablero */
	rect.x = 26;
	rect.y = 65;
	rect.w = 182;
	rect.h = 28;
	
	rect2.x = rect2.y = 0;
	rect2.w = 182;
	rect2.h = 28;
	
	SDL_BlitSurface (images[IMG_BOARD], &rect2, surface, &rect);
	
	/* Redibujar el pedazo de botón de cierre borrado */
	juego_draw_button_close (j->ventana, j->close_frame);
	if (j->resalte == -1) return;
	
	/* Ahora sí, redibujar el resalte */
	rect.y = 46;
	rect.x = resalte_pos[j->resalte];
	rect.w = images[IMG_BIGCOINRED]->w;
	rect.h = images[IMG_BIGCOINRED]->h;
	if (j->turno % 2 == 0) { /* El que empieza siempre es rojo */
		SDL_BlitSurface (images[IMG_BIGCOINRED], NULL, surface, &rect);
	} else {
		SDL_BlitSurface (images[IMG_BIGCOINBLUE], NULL, surface, &rect);
	}
}

void juego_dibujar_tablero (Juego *j) {
	Uint32 color;
	SDL_Surface *surface;
	SDL_Rect rect;
	int g, h;
	
	surface = window_get_surface (j->ventana);
	
	color = SDL_MapRGB (surface->format, 0x00, 0x52, 0x9b);
	
	rect.x = 26;
	rect.y = 46;
	rect.w = 182;
	rect.h = 171;
	
	SDL_FillRect (surface, &rect, color);
	window_update (j->ventana, &rect);
	
	/* Redibujar las fichas de la fila superior */
	/* dibujar las fichas antes del tablero */
	for (g = 0; g < 6; g++) {
		for (h = 0; h < 7; h++) {
			if (j->tablero[g][h] == 0) continue;
			rect.x = tablero_cols[h];
			rect.y = tablero_filas[g];
			
			rect.w = images[IMG_COINBLUE]->w;
			rect.h = images[IMG_COINBLUE]->h;
			
			if (j->tablero[g][h] == 1) {
				SDL_BlitSurface (images[IMG_COINRED], NULL, surface, &rect);
			} else {
				SDL_BlitSurface (images[IMG_COINBLUE], NULL, surface, &rect);
			}
		}
	}
	
	/* Dibujar la ficha en plena animación */
	if (j->num_a > 0) {
		rect.x = tablero_cols[j->animaciones[0].col];
		g = j->animaciones[0].frame / ANIM_VEL;
		rect.y = tablero_filas[g];
		rect.w = images[IMG_COINBLUE]->w;
		rect.h = images[IMG_COINBLUE]->h;
		
		if (j->animaciones[0].color == 1) {
			SDL_BlitSurface (images[IMG_COINRED], NULL, surface, &rect);
		} else {
			SDL_BlitSurface (images[IMG_COINBLUE], NULL, surface, &rect);
		}
	}
	
	rect.x = 26;
	rect.y = 65;
	rect.w = images[IMG_BOARD]->w;
	rect.h = images[IMG_BOARD]->h;
	
	SDL_BlitSurface (images[IMG_BOARD], NULL, surface, &rect);
	
	/* Redibujar el pedazo de botón de cierre borrado */
	juego_draw_button_close (j->ventana, j->close_frame);
	if (j->resalte != -1) {
		/* Ahora sí, redibujar el resalte */
		rect.y = 46;
		rect.x = resalte_pos[j->resalte];
		rect.w = images[IMG_BIGCOINRED]->w;
		rect.h = images[IMG_BIGCOINRED]->h;
		if (j->turno % 2 == 0) { /* El que empieza siempre es rojo */
			SDL_BlitSurface (images[IMG_BIGCOINRED], NULL, surface, &rect);
		} else {
			SDL_BlitSurface (images[IMG_BIGCOINBLUE], NULL, surface, &rect);
		}
	}
	
	/* Dibujar el nombre del jugador local */
	rect.x = 74;
	rect.y = 242;
	rect.w = images[IMG_WINDOW]->w - 74;
	rect.h = nick_image->h;
	
	SDL_BlitSurface (images[IMG_WINDOW], &rect, surface, &rect);
	window_update (j->ventana, &rect);
	
	if (j->win != 0 || j->turno == 42) {
		SDL_BlitSurface (((j->turno - 1) % 2) == j->inicio ? nick_image : nick_image_blue, NULL, surface, &rect);
	} else {
		SDL_BlitSurface ((j->turno % 2) == j->inicio ? nick_image : nick_image_blue, NULL, surface, &rect);
	}
	
	/* Borrar el texto de "conectando" */
	rect.x = 74;
	rect.y = 272;
	rect.w = images[IMG_WINDOW]->w - 74;
	rect.h = text_waiting->h;
	
	SDL_BlitSurface (images[IMG_WINDOW], &rect, surface, &rect);
	window_update (j->ventana, &rect);
	
	/* Dibujar el nick remoto */
	rect.x = 74;
	rect.y = 272;
	rect.w = j->nick_remoto_image->w;
	rect.h = j->nick_remoto_image->h;
	
	if (j->win != 0 || j->turno == 42) {
		SDL_BlitSurface (((j->turno - 1) % 2) != j->inicio ? j->nick_remoto_image : j->nick_remoto_image_blue, NULL, surface, &rect);
	} else {
		SDL_BlitSurface ((j->turno % 2) != j->inicio ? j->nick_remoto_image : j->nick_remoto_image_blue, NULL, surface, &rect);
	}
}

int juego_mouse_motion (Ventana *v, int x, int y) {
	Juego *j;
	int old_resalte;
	
	j = (Juego *) window_get_data (v);
	
	/* Primero, quitar el resalte */
	old_resalte = j->resalte;
	
	j->resalte = -1;
	
	/* Si es nuestro turno, hacer resalte */
	if (y > 65 && y < 217 && x > 26 && x < 208 && (j->turno % 2) == j->inicio && j->estado == NET_READY) {
		/* Está dentro del tablero */
		if (x >= 32 && x < 56 && j->tablero[0][0] == 0) {
			/* Primer fila de resalte */
			j->resalte = 0;
		} else if (x >= 56 && x < 81 && j->tablero[0][1] == 0) {
			j->resalte = 1;
		} else if (x >= 81 && x < 105 && j->tablero[0][2] == 0) {
			j->resalte = 2;
		} else if (x >= 105 && x < 129 && j->tablero[0][3] == 0) {
			j->resalte = 3;
		} else if (x >= 129 && x < 153 && j->tablero[0][4] == 0) {
			j->resalte = 4;
		} else if (x >= 153 && x < 178 && j->tablero[0][5] == 0) {
			j->resalte = 5;
		} else if (x >= 178 && x < 206 && j->tablero[0][6] == 0) {
			j->resalte = 6;
		}
	}
	
	if (old_resalte != j->resalte) {
		juego_dibujar_resalte (j);
	}
	
	/* En caso contrario, buscar si el mouse está en el botón de cierre */
	if (y >= 26 && y < 54 && x >= 192 && x < 220) {
		/* FIXME: Arreglar lo de los botones */
		window_button_mouse_motion (v, JUEGO_BUTTON_CLOSE);
		return TRUE;
	}
	if ((y >= 16) || (x >= 64 && x < 168)) {
		/* El evento cae dentro de la ventana */
		return TRUE;
	}
	
	return FALSE;
}

int juego_mouse_up (Ventana *v, int x, int y) {
	int g, h;
	
	Juego *j;
	j = (Juego *) window_get_data (v);
	
	if (y > 65 && y < 217 && x > 26 && x < 208 && (j->turno % 2) == j->inicio && j->estado == NET_READY && j->num_a == 0) {
		/* Está dentro del tablero */
		h = -1;
		if (x >= 32 && x < 56 && j->tablero[0][0] == 0) {
			/* Primer fila de resalte */
			h = 0;
		} else if (x >= 56 && x < 81 && j->tablero[0][1] == 0) {
			h = 1;
		} else if (x >= 81 && x < 105 && j->tablero[0][2] == 0) {
			h = 2;
		} else if (x >= 105 && x < 129 && j->tablero[0][3] == 0) {
			h = 3;
		} else if (x >= 129 && x < 153 && j->tablero[0][4] == 0) {
			h = 4;
		} else if (x >= 153 && x < 178 && j->tablero[0][5] == 0) {
			h = 5;
		} else if (x >= 178 && x < 206 && j->tablero[0][6] == 0) {
			h = 6;
		}
		
		if (h >= 0) {
			g = 5;
			while (g > 0 && j->tablero[g][h] != 0) g--;
			
			j->tablero[g][h] = (j->turno % 2) + 1;
			
			if (use_sound) Mix_PlayChannel (-1, sounds[SND_DROP], 0);
			
			/* Enviar el turno */
			j->retry = 0;
			enviar_movimiento (j, j->turno, h, g);
			j->last_col = h;
			j->last_fila = g;
			j->turno++;
			j->resalte = -1;
			
			/* Si es un movimiento ganador, cambiar el estado a WAIT_WINNER */
			buscar_ganador (j);
			
			if (j->win != 0 || j->turno == 42) {
				j->estado = NET_WAIT_WINNER;
			}
			
			/* Borrar la ficha del tablero y meterla a la cola de animación */
			j->animaciones[j->num_a].fila = g;
			j->animaciones[j->num_a].col = h;
			j->animaciones[j->num_a].frame = 0;
			j->animaciones[j->num_a].color = j->tablero[g][h];
			j->num_a++;
			
			j->tablero[g][h] = 0;
			
			/* TODO: Redibujar aquí el tablero e invocar a window_update */
			juego_dibujar_tablero (j);
			window_register_timer_events (j->ventana, juego_timer_callback_anim);
			window_enable_timer (j->ventana);
		}
	}
	
	/* Revisar si el evento cae dentro del botón de cierre */
	if (y >= 26 && y < 54 && x >= 192 && x < 220) {
		/* El click cae en el botón de cierre de la ventana */
		window_button_mouse_up (v, JUEGO_BUTTON_CLOSE);
		return TRUE;
	}
	
	if ((y >= 16) || (x >= 64 && x < 168)) {
		/* El evento cae dentro de la ventana */
		return TRUE;
	}
	
	return FALSE;
}

void buscar_ganador (Juego *j) {
	int g, h;
	
	/* Tablero:
	   0 1 2 3 4 5 6
	  ---------------
	5 | | | | | | | |
	  ---------------
	4 | | | | | | | |
	  ---------------
	3 | | | | | | | |
	  ---------------
	2 | | | | | | | |
	  ---------------
	1 | | | | | | | |
	  ---------------
	0 | | | | | | | |
	  ---------------
	 */
	
	/* Buscar por columnas */
	for (g = 0; g < 7; g++) {
		for (h = 0; h < 3; h++) { /* Las 3 primeras filas */
			if (j->tablero[h][g] == j->tablero[h + 1][g] &&
			    j->tablero[h + 1][g] == j->tablero[h + 2][g] &&
			    j->tablero[h + 2][g] == j->tablero[h + 3][g]) {
				/* Hay un ganador */
				if (j->tablero[h][g] != 0) {
					j->win = j->tablero[h][g];
					j->win_col = g;
					j->win_fila = 5 - h;
					j->win_dir = 1;
				}
			}
		}
	}
	
	/* Buscar por filas */
	for (g = 0; g < 4; g++) { /* Las 4 primeras columnas */
		for (h = 0; h < 6; h++) { /* Las 6 filas */
			if (j->tablero[h][g] == j->tablero[h][g + 1] &&
			    j->tablero[h][g + 1] == j->tablero[h][g + 2] &&
			    j->tablero[h][g + 2] == j->tablero[h][g + 3]) {
				/* Hay un ganador */
				if (j->tablero[h][g] != 0) {
					j->win = j->tablero[h][g];
					j->win_col = g;
					j->win_fila = 5 - h;
					j->win_dir = 3;
				}
			}
		}
	}
	
	/* Buscar por diagonales / */
	for (g = 0; g < 4; g++) {
		for (h = 3; h < 6; h++) {
			if (j->tablero[h][g] == j->tablero[h - 1][g + 1] &&
			    j->tablero[h - 1][g + 1] == j->tablero[h - 2][g + 2] &&
			    j->tablero[h - 2][g + 2] == j->tablero[h - 3][g + 3]) {
				if (j->tablero[h][g] != 0) {
					j->win = j->tablero[h][g];
					j->win_col = g;
					j->win_fila = 5 - h;
					j->win_dir = 2;
				}
			}
		}
	}
	
	/* Buscar por diagonales \ */
	for (g = 0; g < 4; g++) {
		for (h = 0; h < 3; h++) {
			if (j->tablero[h][g] == j->tablero[h + 1][g + 1] &&
			    j->tablero[h + 1][g + 1] == j->tablero[h + 2][g + 2] &&
			    j->tablero[h + 2][g + 2] == j->tablero[h + 3][g + 3]) {
				if (j->tablero[h][g] != 0) {
					j->win = j->tablero[h][g];
					j->win_col = g;
					j->win_fila = 5 - h;
					j->win_dir = 4;
				}
			}
		}
	}
	
	if (j->win != 0) {
		if (use_sound) Mix_PlayChannel (-1, sounds[SND_WIN], 0);
	}
}

void recibir_movimiento (Juego *j, int turno, int col, int fila) {
	int g;
	/* Antes de recibir un movimiento, poner temporalmente en el tablero las fichas que se están animando */
	for (g = 0; g < j->num_a; g++) {
		j->tablero[j->animaciones[g].fila][j->animaciones[g].col] = j->animaciones[g].color;
	}
	if (j->turno != turno) {
		/* TODO: Si el turno es 1 menor, es una repetición */
		if (turno == j->turno - 1) {
			/* Buscar un ganador, si lo hay, enviar el ACK_GAME */
			if (j->win != 0) {
				enviar_mov_ack_finish (j, GAME_FINISH_LOST);
				j->estado = NET_CLOSED;
			} else if (j->turno == 42) {
				enviar_mov_ack_finish (j, GAME_FINISH_TIE);
				j->estado = NET_CLOSED;
			} else {
				enviar_mov_ack (j);
			}
		} else {
			fprintf (stderr, "Wrong turn number\n");
			j->last_fin = NET_DISCONNECT_WRONG_TURN;
			goto fin_and_close;
		}
	} else if (j->turno % 2 == j->inicio) {
		fprintf (stderr, "It's our turn, closing the connection\n");
		j->last_fin = NET_DISCONNECT_WRONG_TURN;
		goto fin_and_close;
	} else if (j->tablero[0][col] != 0) {
		/* Tablero lleno, columna equivocada */
		fprintf (stderr, "Wrong movement\n");
		j->last_fin = NET_DISCONNECT_WRONG_MOV;
		goto fin_and_close;
	} else {
		g = 5;
		while (g > 0 && j->tablero[g][col] != 0) g--;
	
		/* Poner la ficha en la posición [g][turno] y avanzar turno */
		if (g != fila) {
			fprintf (stderr, "Wrong movement - Out of sync\n");
			j->last_fin = NET_DISCONNECT_WRONG_MOV;
			goto fin_and_close;
		} else {
			j->tablero[g][col] = (j->turno % 2) + 1;
			j->turno++;
			
			if (use_sound) Mix_PlayChannel (-1, sounds[SND_DROP], 0);
			
			buscar_ganador (j);
			
			/* Buscar un ganador, si lo hay, enviar el ACK_GAME */
			if (j->win != 0) {
				enviar_mov_ack_finish (j, GAME_FINISH_LOST);
				//message_add (MSG_NORMAL, "%s ha ganado la partida", j->nick_remoto);
				j->estado = NET_CLOSED;
			} else if (j->turno == 42) {
				enviar_mov_ack_finish (j, GAME_FINISH_TIE);
				//message_add (MSG_NORMAL, "%s y tú han empatado", j->nick_remoto);
				j->estado = NET_CLOSED;
			} else {
				enviar_mov_ack (j);
			}
			
			/* Quitar la ficha recién recibida por red,
			 * y meterla a la cola de animación */
			j->animaciones[j->num_a].fila = g;
			j->animaciones[j->num_a].col = col;
			j->animaciones[j->num_a].frame = 0;
			j->animaciones[j->num_a].color = j->tablero[g][col];
			j->num_a++;
			
			j->tablero[g][col] = 0;
			
			window_register_timer_events (j->ventana, juego_timer_callback_anim);
			window_enable_timer (j->ventana);
		}
	}
	
	/* Quitar las fichas que se están animando */
	for (g = 0; g < j->num_a; g++) {
		j->tablero[j->animaciones[g].fila][j->animaciones[g].col] = 0;
	}
	
	return;
fin_and_close:
	enviar_fin (j);
	
	/* Cerrar la ventana asociada porque caimos en condición de error */
	if (j->ventana != NULL) {
		/* Destruir la ventana asociada con este juego */
		window_destroy (j->ventana);
		j->ventana = NULL;
	}
}

void juego_draw_button_close (Ventana *v, int frame) {
	SDL_Surface *surface = window_get_surface (v);
	SDL_Rect rect;
	
	/* Dibujar el botón de cierre */
	rect.x = 192;
	rect.y = 26;
	rect.w = images[IMG_BUTTON_CLOSE_UP]->w;
	rect.h = images[IMG_BUTTON_CLOSE_UP]->h;
	
	SDL_BlitSurface (images[IMG_WINDOW], &rect, surface, &rect);
	
	SDL_BlitSurface (images[IMG_BUTTON_CLOSE_UP + frame], NULL, surface, &rect);
	window_update (v, &rect);
}

void recibir_nick (Juego *j, const char *nick) {
	SDL_Color blanco;
	SDL_Color negro;
	SDL_Rect rect;
	SDL_Surface *surface;
	int first_time = 0;
	
	memcpy (j->nick_remoto, nick, sizeof (j->nick_remoto));
	
	surface = window_get_surface (j->ventana);
	
	/* Renderizar el nick del otro jugador */
	blanco.r = blanco.g = blanco.b = 255;
	negro.r = negro.g = negro.b = 0;
	if (j->nick_remoto_image != NULL) {
		SDL_FreeSurface (j->nick_remoto_image);
	} else {
		first_time = 1;
	}
	j->nick_remoto_image = draw_text_with_shadow (ttf16_comiccrazy, 2, j->nick_remoto, blanco, negro);
	
	blanco.r = 0xD5; blanco.g = 0xF1; blanco.b = 0xFF;
	negro.r = 0x33; negro.g = 0x66; negro.b = 0x99;
	if (j->nick_remoto_image_blue != NULL) {
		SDL_FreeSurface (j->nick_remoto_image_blue);
	}
	j->nick_remoto_image_blue = draw_text_with_shadow (ttf16_comiccrazy, 2, j->nick_remoto, blanco, negro);
	
	/* Quitar el timer solo si es la primera vez que recibo el nick */
	if (first_time == 1) {
		window_disable_timer (j->ventana);
	}
	
	/* Borrar con el fondo */
	rect.x = 39;
	rect.y = 235;
	rect.h = 64;
	rect.w = 32;
	
	window_update (j->ventana, &rect);
	
	SDL_BlitSurface (images[IMG_WINDOW], &rect, surface, &rect);
	
	/* En este punto, dibujar ya las piezas de color en orden */
	rect.x = 43;
	rect.y = 239;
	rect.w = images[IMG_COINRED]->w;
	rect.h = images[IMG_COINRED]->h;
	
	if (j->inicio == 0) {
		SDL_BlitSurface (images[IMG_COINRED], NULL, surface, &rect);
	} else {
		SDL_BlitSurface (images[IMG_COINBLUE], NULL, surface, &rect);
	}
	
	rect.x = 43;
	rect.y = 269;
	rect.w = images[IMG_COINRED]->w;
	rect.h = images[IMG_COINRED]->h;
	
	if (j->inicio != 0) {
		SDL_BlitSurface (images[IMG_COINRED], NULL, surface, &rect);
	} else {
		SDL_BlitSurface (images[IMG_COINBLUE], NULL, surface, &rect);
	}
	
	/* Dibujar los nombres en el orden correcto */
	
	/* Dibujar el nombre del jugador local */
	rect.x = 74;
	rect.y = 242;
	rect.w = images[IMG_WINDOW]->w - 74;
	rect.h = nick_image->h;
	
	SDL_BlitSurface (images[IMG_WINDOW], &rect, surface, &rect);
	window_update (j->ventana, &rect);
	
	SDL_BlitSurface ((j->turno % 2) == j->inicio ? nick_image : nick_image_blue, NULL, surface, &rect);
	
	/* Borrar el texto de "conectando" */
	rect.x = 74;
	rect.y = 272;
	rect.w = images[IMG_WINDOW]->w - 74;
	rect.h = text_waiting->h;
	
	SDL_BlitSurface (images[IMG_WINDOW], &rect, surface, &rect);
	window_update (j->ventana, &rect);
	
	/* Dibujar el nick remoto */
	rect.x = 74;
	rect.y = 272;
	rect.w = j->nick_remoto_image->w;
	rect.h = j->nick_remoto_image->h;
	
	SDL_BlitSurface ((j->turno % 2) != j->inicio ? j->nick_remoto_image : j->nick_remoto_image_blue, NULL, surface, &rect);
}

