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

#define NET_CONN_TIMER 1850
#define NET_READY_TIMER 4950

#define FLAG_ACK 0x01
#define FLAG_SYN 0x02
#define FLAG_TRN 0x04
#define FLAG_RST 0x08
#define FLAG_ALV 0x10

typedef struct {
	uint8_t flags;
	uint16_t seq;
	uint16_t ack;
} FF_netbase;

typedef struct {
	uint8_t flags;
	uint16_t seq;
	uint16_t ack;
	uint8_t version;
	char nick[NICK_SIZE];
} FF_syn;

typedef struct {
	uint8_t flags;
	uint16_t seq;
	uint16_t ack;
	uint8_t version;
	char nick[NICK_SIZE];
	uint8_t initial;
} FF_syn_ack;

typedef struct {
	uint8_t flags;
	uint16_t seq;
	uint16_t ack;
} FF_ack;

typedef struct {
	uint8_t flags;
	uint16_t seq;
	uint16_t ack;
	uint8_t turno;
	uint8_t col;
	uint8_t fila;
} FF_trn;

/* Estructura para el mensaje de red */
typedef union {
	FF_netbase base;
	FF_syn syn;
	FF_syn_ack syn_ack;
	FF_ack ack;
	FF_trn trn;
} FF_NET;

/* Los posibles estados en los que se encuentra la partida */
enum {
	/* Acabamos de enviar un SYN para conexión inicial, esperamos el SYN + ACK
	 */
	NET_SYN_SENT = 0,
	/* Estado inicial para una conexión entrante, cuando llega una conexión entrante con SYN activado
	 * Después de recibir el paquete, estamos obligados a enviar un SYN + ACK
	 * En este estado estamos esperando recibir el ACK = 0
	 */
	NET_SYN_RECV,
	
	NET_READY,
	NET_WAIT_TRN_ACK,
	
	NUM_NETSTATE
};

/* Funciones públicas */
int findfour_netinit (int);

void process_netevent (void);

void conectar_con (Juego *juego, const char *, const char *, const int);
void enviar_movimiento (Juego *, int, int);

#endif /* __NETPLAY_H__ */

