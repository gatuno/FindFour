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
#include "juego.h"

#define NET_CONN_TIMER 3400
#define NET_READY_TIMER 4950
#define NET_MCAST_TIMER 12000

enum {
	TYPE_SYN = 1,
	TYPE_RES_SYN,
	TYPE_TRN,
	TYPE_TRN_ACK,
	TYPE_TRN_ACK_GAME,
	
	TYPE_FIN = 64,
	TYPE_FIN_ACK
};

#ifndef IN6_IS_ADDR_V4MAPPED
#define IN6_IS_ADDR_V4MAPPED(a) \
       ((((a)->s6_words[0]) == 0) && \
        (((a)->s6_words[1]) == 0) && \
        (((a)->s6_word[2]) == 0) && \
        (((a)->s6_word[3]) == 0) && \
        (((a)->s6_word[4]) == 0) && \
        (((a)->s6_word[5]) == 0xFFFF))
#endif

/* Estructura para el mensaje de red */
typedef struct {
	uint8_t version;
	uint8_t type;
	uint16_t local, remote;
	union {
		struct {
			char nick[NICK_SIZE];
			uint8_t initial;
		};
		struct {
			uint8_t turno, col, fila;
		};
		struct {
			uint8_t turn_ack;
			uint8_t win;
		};
		struct {
			uint8_t fin;
		};
	};
} FFMessageNet;

/* Los posibles estados en los que se encuentra la partida */
enum {
	/* Acabamos de enviar un SYN para conexión inicial, esperamos por la respuesta RES + SYN */
	NET_SYN_SENT = 0,
	
	NET_READY,
	
	/* Espero la confirmación de turno */
	NET_WAIT_ACK,
	
	/* Espero la confirmación de turno y que el otro admita que gané (o empatamos) */
	NET_WAIT_WINNER,
	
	/* Envié un fin, espero la confirmación de FIN */
	NET_WAIT_CLOSING,
	
	/* Conexión cerrada, para mostrar cuando alguien gana */
	NET_CLOSED,
	
	NUM_NETSTATE
};

/* Razones por la cuál desconectar */
enum {
	NET_DISCONNECT_NETERROR = 0, /* Demasiados intentos de repetición y no hay respuesta */
	NET_DISCONNECT_UNKNOWN_START, /* Error al recibir quién empieza */
	NET_DISCONNECT_WRONG_TURN, /* Número de turno equivocado */
	NET_DISCONNECT_WRONG_MOV, /* Movimiento equivocado, AKA columna llena */
	
	NET_USER_QUIT /* El usuario abandonó la partida */
};

/* Razones de confirmación de turno y finalización de juego */
enum {
	GAME_FINISH_TIE = 1,
	GAME_FINISH_LOST
};

/* Funciones públicas */
int findfour_netinit (int);
void findfour_netclose (void);

int sockaddr_cmp (struct sockaddr *x, struct sockaddr *y);

void process_netevent (void);

void conectar_con (Juego *, const char *, const char *, const int);
void conectar_con_sockaddr (Juego *, const char *, struct sockaddr *, socklen_t);
void enviar_movimiento (Juego *, int, int, int);
void enviar_mov_ack (Juego *);
void enviar_mov_ack_finish (Juego *, int);
void enviar_fin (Juego *);

void enviar_broadcast_game (char *nick);

#endif /* __NETPLAY_H__ */

