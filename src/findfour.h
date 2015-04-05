/*
 * findfour.h
 * This file is part of Find Four
 *
 * Copyright (C) 2015 - Félix Arreola Rodríguez
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __FINDFOUR_H__
#define __FINDFOUR_H__

#include <stdlib.h>

/* Para el manejo de red */
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

#define NICK_SIZE 16

#define RANDOM(x) ((int) (x ## .0 * rand () / (RAND_MAX + 1.0)))

/* Estructuras */
typedef struct _Juego {
	int id;
	
	int x, y;
	int w, h;
	
	struct _Juego *prev, *next;
	
	int inicio;
	int turno;
	int resalte;
	
	int tablero[6][7];
	
	struct sockaddr_storage cliente;
	socklen_t tamsock;
	
	char nick[NICK_SIZE];
	
	int estado;
	/* Campo last_time response */
} Juego;

/* Funciones públicas */
Juego *crear_ventana (void);

extern Juego *primero;

#endif /* __FINDFOUR_H__ */

