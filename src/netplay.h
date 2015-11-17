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

#define FLAG_ACK 0x01
#define FLAG_SYN 0x02
#define FLAG_TRN 0x04
#define FLAG_FIN 0x08
#define FLAG_ALV 0x10
#define FLAG_MCG 0x20

#ifndef IN6_IS_ADDR_V4MAPPED
#define IN6_IS_ADDR_V4MAPPED(a) \
       ((((a)->s6_words[0]) == 0) && \
        (((a)->s6_words[1]) == 0) && \
        (((a)->s6_word[2]) == 0) && \
        (((a)->s6_word[3]) == 0) && \
        (((a)->s6_word[4]) == 0) && \
        (((a)->s6_word[5]) == 0xFFFF))
#endif

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

typedef struct {
	uint8_t flags;
	uint16_t seq;
	uint16_t ack;
	uint8_t fin;
} FF_fin;

typedef struct {
	uint8_t flags;
	uint8_t version;
	char nick[NICK_SIZE];
} FF_broadcast_game;

/* Estructura para el mensaje de red */
typedef union {
	FF_netbase base;
	FF_syn syn;
	FF_syn_ack syn_ack;
	FF_ack ack;
	FF_trn trn;
	FF_fin fin;
	FF_broadcast_game bgame;
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
	
	/* Conexión que se está cerrando, esperamos por un último FIN + ACK si es que llega */
	NET_WAIT_CLOSING,
	
	/* Conexión que sigue en linea, pero esperamos a que el otro extremo nos envié el anuncio de ganador */
	NET_WAIT_WINNER,
	
	/* Conexión que espera el FIN + ACK del ganador para poder cerrar la conexión */
	NET_WAIT_WINNER_ACK,
	
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
	
	NET_USER_QUIT, /* El usuario abandonó la partida */
	
	NET_DISCONNECT_YOUWIN = 64,
	NET_DISCONNECT_YOULOST,
	NET_DISCONNECT_TIE
};

/* Funciones públicas */
int findfour_netinit (int);
void findfour_netclose (void);

void process_netevent (void);

void conectar_con (Juego *, const char *, const char *, const int);
void conectar_con_sockaddr (Juego *, const char *, struct sockaddr *, socklen_t);
void enviar_movimiento (Juego *, int, int);
void enviar_fin (Juego *, FF_NET *, int);

void enviar_broadcast_game (char *nick);

#endif /* __NETPLAY_H__ */

