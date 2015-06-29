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
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

#include "findfour.h"

/* Estructuras */
typedef struct _Juego {
	Ventana ventana;
	
	/* Quién empieza el juego y el turno actual */
	int inicio;
	int turno;
	int resalte;
	
	/* El tablero, 0 = nada, 1 = ficha roja, 2 = ficha azul */
	int tablero[6][7];
	
	/* El botón de cierre */
	int close_frame; /* ¿El refresh es necesario? */
	
	/* La dirección del cliente */
	struct sockaddr_storage cliente;
	socklen_t tamsock;
	
	/* El nick del otro jugador */
	char nick[NICK_SIZE];
	int win, win_col, win_fila, win_dir;
	
	/* Estado del protocolo de red */
	int estado;
	int retry;
	Uint32 last_response;
	uint16_t seq, ack;
	char buffer_send[256];
	size_t len_send;
} Juego;

/* Funciones públicas */
Juego *crear_juego (void);
void eliminar_juego (Juego *);
int recibir_movimiento (Juego *, int turno, int col, int fila, int *);
void buscar_ganador (Juego *j);

#endif /* __JUEGO_H__ */

