/*
 * juego.h
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

#ifndef __JUEGO_H__
#define __JUEGO_H__

/* Para el manejo de red */
#ifdef __MINGW32__
#	include <winsock2.h>
#	include <ws2tcpip.h>
#else
#	include <sys/socket.h>
#	include <netinet/in.h>
#endif
#include <sys/types.h>

#include <SDL.h>

#include "findfour.h"

typedef struct {
	int fila, col, frame, color;
} Anim;

/* Estructuras */
typedef struct _Juego {
	Ventana ventana;
	
	/* Quién empieza el juego y el turno actual */
	int inicio;
	int turno;
	int resalte;
	int timer;
	
	/* El tablero, 0 = nada, 1 = ficha roja, 2 = ficha azul */
	int tablero[6][7];
	
	/* El botón de cierre */
	int close_frame; /* ¿El refresh es necesario? */
	
	/* La dirección del otro peer */
	struct sockaddr_storage peer;
	socklen_t peer_socklen;
	
	/* El nick del otro jugador */
	char nick_remoto[NICK_SIZE];
	SDL_Surface *nick_remoto_image, *nick_remoto_image_blue;
	int win, win_col, win_fila, win_dir;
	
	/* Estado del protocolo de red */
	int estado;
	int retry;
	Uint32 last_response;
	uint16_t local, remote;
	int last_fila, last_col, last_fin;
	
	/* Para animación */
	int num_a;
	Anim animaciones[4];
} Juego;

/* Funciones públicas */
Juego *crear_juego (int top_window);
void eliminar_juego (Juego *);
void recibir_movimiento (Juego *, int turno, int col, int fila);
void buscar_ganador (Juego *j);
void recibir_nick (Juego *, const char *);

#endif /* __JUEGO_H__ */

