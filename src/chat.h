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

#include <SDL.h>

/* Para el manejo de red */
#ifdef __MINGW32__
#	include <winsock2.h>
#	include <ws2tcpip.h>
#else
#	include <sys/socket.h>
#	include <netinet/in.h>
#endif
#include <sys/types.h>

#include "findfour.h"
#include "ventana.h"

enum {
	CHAT_LIST_MCAST = 0,
	
	CHAT_LIST_RECENT,
	
	NUM_CHAT_LIST
};

/* Estructuras */
typedef struct _BuddyMCast {
	struct _BuddyMCast *next;
	
	char nick[NICK_SIZE];
	
	/* La dirección del cliente */
	struct sockaddr_storage cliente;
	
	/* El nombre del cliente renderizado */
	SDL_Surface *nick_chat;
	
	socklen_t tamsock;
	Uint32 last_seen;
} BuddyMCast;

typedef struct _RecentPlay {
	struct _RecentPlay *next;
	
	char nick[NICK_SIZE];
	
	/* La dirección del cliente */
	char buffer[256];
	SDL_Surface *text;
} RecentPlay;

typedef struct _Chat {
	Ventana *ventana;
	
	int close_frame;
	
	/* Los 8 buddys buttons */
	int buddys_frame[8];
	
	int list_display;
	int list_offset;
	
	Uint32 azul1;
	
	/* Los buddys encontrados por red */
	BuddyMCast *buddy_mcast;
	int buddy_mcast_count;
	SDL_Surface *mcast_text;
	
	/* Las partidas recientes */
	RecentPlay *buddy_recent;
	int buddy_recent_count;
	SDL_Surface *recent_text;
} Chat;

void inicializar_chat (void);
void buddy_list_mcast_add (const char *nick, struct sockaddr *direccion, socklen_t tamsock);
void buddy_list_mcast_remove (struct sockaddr *direccion, socklen_t tamsock);
void buddy_list_mcast_clean (Uint32 timestamp);
void buddy_list_recent_add (const char *texto);
void show_chat (void);

#endif /* __CHAT_H__ */

