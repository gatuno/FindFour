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

#define NET_CONN_TIMER 3400
#define NET_MCAST_TIMER 12000
#define NET_MCAST_TIMEOUT 120000

enum {
	TYPE_SYN = 1,
	TYPE_RES_SYN,
	TYPE_TRN,
	TYPE_TRN_ACK,
	TYPE_TRN_ACK_GAME,
	TYPE_KEEP_ALIVE,
	TYPE_KEEP_ALIVE_ACK,
	
	TYPE_MCAST_ANNOUNCE = 32,
	TYPE_MCAST_FIN,
	
	TYPE_FIN = 64,
	TYPE_FIN_ACK
};

#ifndef IN6_IS_ADDR_V4MAPPED
#define IN6_IS_ADDR_V4MAPPED(a) \
       ((((a)->s6_addr[0]) == 0) && \
        (((a)->s6_addr[1]) == 0) && \
        (((a)->s6_addr[2]) == 0) && \
        (((a)->s6_addr[3]) == 0) && \
        (((a)->s6_addr[4]) == 0) && \
        (((a)->s6_addr[5]) == 0) && \
        (((a)->s6_addr[6]) == 0) && \
        (((a)->s6_addr[7]) == 0) && \
        (((a)->s6_addr[8]) == 0) && \
        (((a)->s6_addr[9]) == 0) && \
        (((a)->s6_addr[10]) == 0xFF) && \
        (((a)->s6_addr[11]) == 0xFF))
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
void enviar_end_broadcast_game (void);

#ifdef __MINGW32__
SOCKET findfour_get_socket4 (void);
#else
int findfour_get_socket4 (void);
#endif

#endif /* __NETPLAY_H__ */

