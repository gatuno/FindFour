/*
 * netplay.h
 * This file is part of FindFour
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

#ifndef __NETPLAY_H__
#define __NETPLAY_H__

#include <stdint.h>

#include "findfour.h"

/* Estructuras de red */
typedef struct _FF_SYN {
	uint8_t version;
	char nick[NICK_SIZE];
} FF_SYN;

typedef struct _FF_ACK {
	uint8_t ack;
	uint8_t fila;
} FF_ACK;

typedef struct _FF_SYN_ACK {
	FF_SYN syn;
	FF_ACK ack;
} FF_SYN_ACK;

typedef struct _FF_SYN_TRN {
	uint8_t inicio;
} FF_SYN_TRN;

typedef struct _FF_TRN {
	uint8_t turno;
	uint8_t columna;
} FF_TRN;

typedef struct _FF_TRN_ACK {
	FF_TRN trn;
	FF_ACK ack;
} FF_TRN_ACK;

typedef union {
	uint8_t flags; /* Para forzar el tamaño a uint8_t */
	struct {
		unsigned int ack : 1;
		unsigned int syn : 1;
		unsigned int trn : 1;
		unsigned int rst : 1;
	};
} FF_Flags;

/* Estructura para el mensaje de red */
typedef struct _FF_NET {
	FF_Flags flags;
	union {
		FF_SYN syn;
		FF_ACK ack;
		FF_SYN_ACK syn_ack;
		FF_SYN_TRN syn_trn;
		FF_TRN trn;
		FF_TRN_ACK trn_ack;
	};
} FF_NET;

/* Los posibles estados en los que se encuentra la partida */
enum {
	/* Estado inicial, cuando llega una conexión entrante con SYN activado
	 * Después de recibir el paquete, estamos obligados a enviar un SYN + ACK
	 * En este estado estamos esperando recibir el ACK = 0
	 */
	NET_WAIT_ACK_0 = 0,
	
	/* A la espera por la confirmación del turno inicial */
	NET_WAIT_SYN_TRN_ACK,
	
	NET_READY,
	NET_WAIT_ACK,
	
	NUM_NETSTATE
};

/* Funciones públicas */
int findfour_netinit (void);

void process_netevent (int fd);

#endif /* __NETPLAY_H__ */

