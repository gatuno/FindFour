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

#include "findfour.h"
#include "juego.h"
#include "netplay.h"
#include "cp-button.h"

int juego_mouse_down (Juego *j, int x, int y, int **button_map);
int juego_mouse_motion (Juego *j, int x, int y, int **button_map);
int juego_mouse_up (Juego *j, int x, int y, int **button_map);
void juego_draw (Juego *j, SDL_Surface *screen);

/* Algunas constantes */
const int tablero_cols[7] = {32, 56, 81, 105, 129, 153, 178};
const int tablero_filas[6] = {69, 93, 117, 141, 166, 190};

Juego *crear_juego (void) {
	Juego *j;
	static int start = 0;
	
	/* Crear una nueva ventana */
	j = (Juego *) malloc (sizeof (Juego));
	
	j->ventana.prev = NULL;
	j->ventana.next = primero;
	j->ventana.w = 232; /* FIXME: Arreglar esto */
	j->ventana.h = 324;
	
	j->ventana.tipo = WINDOW_GAME;
	j->ventana.mostrar = TRUE;
	
	j->ventana.mouse_down = juego_mouse_down;
	j->ventana.mouse_motion = juego_mouse_motion;
	j->ventana.mouse_up = juego_mouse_up;
	j->ventana.draw = juego_draw;
	
	j->turno = 0;
	j->inicio = -1;
	j->resalte = -1;
	j->close_frame = IMG_BUTTON_CLOSE_UP;
	start += 20;
	j->ventana.x = j->ventana.y = start;
	
	j->seq = j->ack = 0;
	j->retry = 0;
	
	/* Vaciar el tablero */
	memset (j->tablero, 0, sizeof (int[6][7]));
	
	if (primero == NULL) {
		primero = ultimo = (Ventana *) j;
	} else {
		primero->prev = (Ventana *) j;
		primero = (Ventana *) j;
	}
	
	return j;
}

void eliminar_juego (Juego *j) {
	/* Desligar completamente */
	Ventana *v = (Ventana *) j;;
	if (v->prev != NULL) {
		v->prev->next = v->next;
	} else {
		primero = v->next;
	}
	
	if (v->next != NULL) {
		v->next->prev = v->prev;
	} else {
		ultimo = v->prev;
	}
	
	/* Si hay algún indicativo a estos viejos botones, eliminarlo */
	if (cp_old_map == &(j->close_frame)) {
		cp_old_map = NULL;
	}
	if (cp_last_button == &(j->close_frame)) {
		cp_last_button = NULL;
	}
	
	stop_drag ((Ventana *) j);
	free (j);
}

int juego_mouse_down (Juego *j, int x, int y, int **button_map) {
	if (j->ventana.mostrar == FALSE) return FALSE;
	if (x >= 64 && x < 168 && y < 22) {
		/* Click por el agarre */
		start_drag ((Ventana *) j, x, y);
		return TRUE;
	} else if (y >= 26 && y < 54 && x >= 192 && x < 220) {
		/* El click cae en el botón de cierre de la ventana */
		*button_map = &(j->close_frame);
		return TRUE;
	} else if (y >= 16) {
		/* El evento cae dentro de la ventana */
		return TRUE;
	}
	
	return FALSE;
}

int juego_mouse_motion (Juego *j, int x, int y, int **button_map) {
	if (j->ventana.mostrar == FALSE) return FALSE;
	/* Primero, quitar el resalte */
	j->resalte = -1;
	
	/* Si es nuestro turno, hacer resalte */
	if (y > 65 && y < 217 && x > 26 && x < 208 && (j->turno % 2) == j->inicio) {
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
	
	/* En caso contrario, buscar si el mouse está en el botón de cierre */
	if (y >= 26 && y < 54 && x >= 192 && x < 220) {
		*button_map = &(j->close_frame);
		return TRUE;
	}
	if ((y >= 16) || (x >= 64 && x < 168)) {
		/* El evento cae dentro de la ventana */
		return TRUE;
	}
	
	return FALSE;
}

int juego_mouse_up (Juego *j, int x, int y, int **button_map) {
	int g, h;
	
	if (j->ventana.mostrar == FALSE) return FALSE;
	
	if (y > 65 && y < 217 && x > 26 && x < 208 && (j->turno % 2) == j->inicio) {
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
			
			/* Poner la ficha en la posición [g][h] y avanzar turno */
			j->tablero[g][h] = (j->turno % 2) + 1;
			
			/* Enviar el turno */
			enviar_movimiento (j, h, g);
			j->resalte = -1;
		}
	}
	
	/* Revisar si el evento cae dentro del botón de cierre */
	if (y >= 26 && y < 54 && x >= 192 && x < 220) {
		/* El click cae en el botón de cierre de la ventana */
		*button_map = &(j->close_frame);
		if (cp_button_up (*button_map)) {
			/* Quitar esta ventana */
			printf ("Me pidieron quitar esta ventana\n");
			enviar_fin (j, NULL, NET_USER_QUIT);
		}
	}
	
	if ((y >= 16) || (x >= 64 && x < 168)) {
		/* El evento cae dentro de la ventana */
		return TRUE;
	}
	
	return FALSE;
}

void juego_draw (Juego *j, SDL_Surface *screen) {
	SDL_Rect rect;
	int g, h;
	
	rect.x = j->ventana.x;
	rect.y = j->ventana.y;
	rect.w = j->ventana.w;
	rect.h = j->ventana.h;
	
	SDL_BlitSurface (images[IMG_WINDOW], NULL, screen, &rect);
	
	/* Dibujar el botón de cierre */
	rect.x = j->ventana.x + 192;
	rect.y = j->ventana.y + 26;
	rect.w = images[IMG_BUTTON_CLOSE_UP]->w;
	rect.h = images[IMG_BUTTON_CLOSE_UP]->h;
	
	SDL_BlitSurface (images[j->close_frame], NULL, screen, &rect);
	
	/* dibujar las fichas antes del tablero */
	for (g = 0; g < 6; g++) {
		for (h = 0; h < 7; h++) {
			if (j->tablero[g][h] == 0) continue;
			rect.x = j->ventana.x + tablero_cols[h];
			rect.y = j->ventana.y + tablero_filas[g];
			
			rect.w = images[IMG_COINBLUE]->w;
			rect.h = images[IMG_COINBLUE]->h;
			
			if (j->tablero[g][h] == 1) {
				SDL_BlitSurface (images[IMG_COINRED], NULL, screen, &rect);
			} else {
				SDL_BlitSurface (images[IMG_COINBLUE], NULL, screen, &rect);
			}
		}
	}
	
	rect.x = j->ventana.x + 26;
	rect.y = j->ventana.y + 65;
	rect.w = images[IMG_BOARD]->w;
	rect.h = images[IMG_BOARD]->h;
	
	SDL_BlitSurface (images[IMG_BOARD], NULL, screen, &rect);
	
	if (j->resalte >= 0) {
		/* Dibujar la ficha resaltada en la columna correspondiente */
		rect.x = j->ventana.x;
		rect.y = j->ventana.y + 46;
		switch (j->resalte) {
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
		if (j->turno % 2 == 0) { /* El que empieza siempre es rojo */
			SDL_BlitSurface (images[IMG_BIGCOINRED], NULL, screen, &rect);
		} else {
			SDL_BlitSurface (images[IMG_BIGCOINBLUE], NULL, screen, &rect);
		}
	}
}
