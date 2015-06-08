/*
 * chat.h
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

#ifndef __CHAT_H__
#define __CHAT_H__

/* Para el manejo de red */
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

#include "findfour.h"

enum {
	CHAT_LIST_MCAST = 0,
	
	NUM_CHAT_LIST
};

/* Estructuras */
typedef struct _BuddyMCast {
	struct _BuddyMCast *next;
	
	char nick[NICK_SIZE];
	
	/* La dirección del cliente */
	struct sockaddr_storage cliente;
	socklen_t tamsock;
} BuddyMCast;

typedef struct _Chat {
	Ventana ventana;
	
	/* El botón de cierre */
	int close_frame; /* ¿El refresh es necesario? */
	
	/* El bóton de flecha hacia arriba y hacia abajo */
	int up_frame, down_frame;
	
	/* El botón de buddy locales */
	int broadcast_list_frame;
	
	/* Los 8 buddys buttons */
	int buddys[8];
	
	int list_display;
	int list_offset;
	
	/* Los buddys encontrados por red */
	BuddyMCast *buddy_mcast;
	int buddy_mcast_count;
} Chat;

void inicializar_chat (void);
void buddy_list_mcast_add (const char *nick, struct sockaddr *direccion, socklen_t tamsock);
void show_chat (void);

#endif /* __CHAT_H__ */

