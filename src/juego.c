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
	
	j->ventana.mouse_down = (FindWindowMouseFunc) juego_mouse_down;
	j->ventana.mouse_motion = (FindWindowMouseFunc) juego_mouse_motion;
	j->ventana.mouse_up = (FindWindowMouseFunc) juego_mouse_up;
	j->ventana.draw = (FindWindowDraw) juego_draw;
	j->ventana.key_down = NULL;
	j->ventana.key_up = NULL;
	
	j->turno = 0;
	j->win = 0;
	j->inicio = -1;
	j->resalte = -1;
	j->timer = 0;
	j->close_frame = IMG_BUTTON_CLOSE_UP;
	start += 20;
	j->ventana.x = j->ventana.y = start;
	
	j->seq = j->ack = 0;
	j->retry = 0;
	j->estado = NET_CLOSED;
	
	/* Vaciar el tablero */
	memset (j->tablero, 0, sizeof (int[6][7]));
	
	/* Vaciar el nick del otro jugador */
	memset (j->nick, 0, sizeof (j->nick));
	j->nick_remoto_image = NULL;
	
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
	Ventana *v = (Ventana *) j;
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
	
	if (j->nick_remoto_image != NULL) SDL_FreeSurface (j->nick_remoto_image);
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
	
	if (y > 65 && y < 217 && x > 26 && x < 208 && (j->turno % 2) == j->inicio && j->estado == NET_READY) {
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
			if (j->estado != NET_CLOSED) {
				enviar_fin (j, NULL, NET_USER_QUIT);
			} else {
				eliminar_juego (j);
				return TRUE;
			}
		}
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
					printf ("// Ganador de búsqueda: col %i, %i\n", g, h);
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
					printf ("\\\\ Ganador de búsqueda: col %i, %i\n", g, h);
					j->win_dir = 4;
				}
			}
		}
	}
	
	if (j->win != 0) {
		printf ("Tenemos ganador %i!!!!!. Turno: %i, inicial: %i\n", j->win, j->turno, j->inicio);
		printf ("Col: %i, Fila: %i\n", j->win_col, j->win_fila);
		if ((j->win - 1) != j->inicio) {
			printf ("Ellos ganaron\n");
		} else {
			printf ("Nosotros ganamos\n");
		}
		j->estado = NET_WAIT_WINNER;
		if ((j->turno % 2) == j->inicio) {
			printf ("Yo debo informar el gane al otro\n");
			enviar_fin (j, NULL, ((j->win - 1) != j->inicio) ? NET_DISCONNECT_YOUWIN : NET_DISCONNECT_YOULOST);
		}
	}
	if (j->turno == 42) {
		/* Llegamos al tope del tablero, */
		printf ("Tablero lleno\n");
		j->estado = NET_WAIT_WINNER;
		if ((j->turno % 2) == j->inicio) {
			printf ("Yo le aviso\n");
			enviar_fin (j, NULL, NET_DISCONNECT_TIE);
		}
	}
}

int recibir_movimiento (Juego *j, int turno, int col, int fila, int *fin) {
	int g;
	if (j->turno != turno) {
		printf ("Número de turno equivocado\n");
		*fin = NET_DISCONNECT_WRONG_TURN;
		return -1;
	} else if (j->turno % 2 == j->inicio) {
		printf ("Es nuestro turno, cerrar esta conexión\n");
		*fin = NET_DISCONNECT_WRONG_TURN;
		return -1;
	} else if (j->tablero[0][col] != 0) {
		/* Tablero lleno, columna equivocada */
		printf ("Columna llena, no deberias poner fichas ahí\n");
		*fin = NET_DISCONNECT_WRONG_MOV;
		return -1;
	} else {
		g = 5;
		while (g > 0 && j->tablero[g][col] != 0) g--;
	
		/* Poner la ficha en la posición [g][turno] y avanzar turno */
		if (g != fila) {
			printf ("La ficha de mi lado cayó en la fila incorrecta\n");
			*fin = NET_DISCONNECT_WRONG_MOV;
			return -1;
		} else {
			j->tablero[g][col] = (j->turno % 2) + 1;
			j->turno++;
			return 0;
		}
	}
}

void juego_draw (Juego *j, SDL_Surface *screen) {
	SDL_Rect rect, rect2;
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
	
	/* Los botones de carga */
	if (j->estado == NET_SYN_SENT) {
		rect.x = j->ventana.x + 39;
		rect.y = j->ventana.y + 235;
		rect.w = 32;
		rect.h = 32;
	
		rect2.x = (j->timer % 19) * 32;
		rect2.y = 0;
		rect2.w = rect2.h = 32;
	
		SDL_BlitSurface (images[IMG_LOADING], &rect2, screen, &rect);
	
		rect.x = j->ventana.x + 39;
		rect.y = j->ventana.y + 265;
		rect.w = 32;
		rect.h = 32;
	
		rect2.x = (j->timer % 19) * 32;
		rect2.y = 0;
		rect2.w = rect2.h = 32;
		
		SDL_BlitSurface (images[IMG_LOADING], &rect2, screen, &rect);
	} else {
		/* Dibujar las fichas de colores */
		rect.x = j->ventana.x + 43;
		rect.y = j->ventana.y + 239;
		rect.w = images[IMG_COINRED]->w;
		rect.h = images[IMG_COINRED]->h;
		
		if (j->inicio == 0) {
			SDL_BlitSurface (images[IMG_COINRED], NULL, screen, &rect);
		} else {
			SDL_BlitSurface (images[IMG_COINBLUE], NULL, screen, &rect);
		}
		
		rect.x = j->ventana.x + 43;
		rect.y = j->ventana.y + 269;
		rect.w = images[IMG_COINRED]->w;
		rect.h = images[IMG_COINRED]->h;
		
		if (j->inicio != 0) {
			SDL_BlitSurface (images[IMG_COINRED], NULL, screen, &rect);
		} else {
			SDL_BlitSurface (images[IMG_COINBLUE], NULL, screen, &rect);
		}
	}
	
	/* Dibujar el nombre del jugador local */
	rect.x = j->ventana.x + 74;
	rect.y = j->ventana.y + 242;
	rect.w = nick_image->w;
	rect.h = nick_image->h;
	
	SDL_BlitSurface (nick_image, NULL, screen, &rect);
	
	/* Dibujamos el nick remoto, sólo si no es un SYN inicial */
	if (j->estado != NET_SYN_SENT) {
		if (j->nick_remoto_image == NULL) {
			printf ("Error!!!!!!. Estado = %i\n", j->estado);
		}
		rect.x = j->ventana.x + 74;
		rect.y = j->ventana.y + 272;
		rect.w = j->nick_remoto_image->w;
		rect.h = j->nick_remoto_image->h;
		
		SDL_BlitSurface (j->nick_remoto_image, NULL, screen, &rect);
	}
	
	j->timer++;
	if (j->timer >= 19) j->timer = 0;
	
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
		if (j->turno % 2 == 0) { /* El que empieza siempre es rojo */
			SDL_BlitSurface (images[IMG_BIGCOINRED], NULL, screen, &rect);
		} else {
			SDL_BlitSurface (images[IMG_BIGCOINBLUE], NULL, screen, &rect);
		}
	}
	
	if (j->win != 0) {
		switch (j->win_dir) {
			case 1:
				rect.w = images[IMG_WIN_1]->w;
				rect.h = images[IMG_WIN_1]->h;
				rect.x = j->ventana.x + 28 + (j->win_col * 24);
				rect.y = j->ventana.y + 65 + ((5 - j->win_fila) * 24);
				
				SDL_BlitSurface (images[IMG_WIN_1], NULL, screen, &rect);
				
				break;
			case 3:
				rect.w = images[IMG_WIN_3]->w;
				rect.h = images[IMG_WIN_3]->h;
				rect.x = j->ventana.x + 28 + (j->win_col * 24);
				rect.y = j->ventana.y + 65 + ((5 - j->win_fila) * 24);
				
				SDL_BlitSurface (images[IMG_WIN_3], NULL, screen, &rect);
				break;
			case 2:
				/* Estas diagonales apuntan a la esquina inferior izquierda */
				rect.w = images[IMG_WIN_2]->w;
				rect.h = images[IMG_WIN_2]->h;
				rect.x = j->ventana.x + 28 + (j->win_col * 24);
				rect.y = j->ventana.y + 65 + ((5 - j->win_fila - 3) * 24);
				
				SDL_BlitSurface (images[IMG_WIN_2], NULL, screen, &rect);
				break;
			case 4:
				/* Esta digonal apunta a la esquina superior izquierda */
				rect.w = images[IMG_WIN_4]->w;
				rect.h = images[IMG_WIN_4]->h;
				rect.x = j->ventana.x + 28 + (j->win_col * 24);
				rect.y = j->ventana.y + 65 + ((5 - j->win_fila) * 24);
				
				SDL_BlitSurface (images[IMG_WIN_4], NULL, screen, &rect);
				break;
		}
	}
}

void recibir_nick (Juego *j, const char *nick) {
	SDL_Color blanco;
	
	memcpy (j->nick, nick, sizeof (j->nick));
	
	/* Renderizar el nick del otro jugador */
	blanco.r = blanco.g = blanco.b = 255;
	j->nick_remoto_image = TTF_RenderUTF8_Blended (tt16_comiccrazy, j->nick, blanco);
}
