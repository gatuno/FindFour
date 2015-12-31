/*
 * chat.c
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <SDL.h>
#include <SDL_ttf.h>

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
#include "chat.h"
#include "cp-button.h"
#include "netplay.h"
#include "juego.h"

int chat_mouse_down (Chat *j, int x, int y, int **button_map);
int chat_mouse_motion (Chat *j, int x, int y, int **button_map);
int chat_mouse_up (Chat *j, int x, int y, int **button_map);
void chat_draw (Chat *j, SDL_Surface *screen);

Uint32 azul1 = 0;
static Chat *static_chat = NULL;

void inicializar_chat (void) {
	Chat *c;
	int g;
	
	c = (Chat *) malloc (sizeof (Chat));
	
	c->ventana.w = 232; /* FIXME: Arreglar esto */
	c->ventana.h = 324;
	c->ventana.x = c->ventana.y = 0;
	
	c->ventana.tipo = WINDOW_CHAT;
	c->ventana.mostrar = TRUE;
	
	c->ventana.mouse_down = (FindWindowMouseFunc) chat_mouse_down;
	c->ventana.mouse_motion = (FindWindowMouseFunc) chat_mouse_motion;
	c->ventana.mouse_up = (FindWindowMouseFunc) chat_mouse_up;
	c->ventana.draw = (FindWindowDraw) chat_draw;
	c->ventana.key_down = NULL;
	c->ventana.key_up = NULL;
	
	c->close_frame = IMG_BUTTON_CLOSE_UP;
	c->up_frame = IMG_BUTTON_ARROW_1_UP;
	c->down_frame = IMG_BUTTON_ARROW_2_UP;
	c->broadcast_list_frame = IMG_BUTTON_LIST_UP;
	
	for (g = 0; g < 8; g++) {
		c->buddys[g] = IMG_SHADOW_UP;
	}
	
	c->buddy_mcast = NULL;
	c->buddy_mcast_count = 0;
	
	c->list_display = CHAT_LIST_MCAST;
	c->list_offset = 0;
	
	c->ventana.prev = NULL;
	c->ventana.next = get_first_window ();
	
	if (get_first_window () == NULL) {
		set_first_window ((Ventana *) c);
		set_last_window ((Ventana *) c);
	} else {
		get_first_window ()->prev = (Ventana *) c;
		set_first_window ((Ventana *) c);
	}
	
	static_chat = c;
}

int chat_mouse_down (Chat *c, int x, int y, int **button_map) {
	int g;
	if (c->ventana.mostrar == FALSE) return FALSE;
	
	if (x >= 64 && x < 168 && y < 22) {
		/* Click por el agarre */
		start_drag ((Ventana *) c, x, y);
		return TRUE;
	}
	if (y >= 30 && y < 58 && x >= 190 && x < 218) {
		/* El click cae en el botón de cierre de la ventana */
		*button_map = &(c->close_frame);
		return TRUE;
	}
	if (x >= 190 && x < 218 && y >= 61 && y < 89) {
		*button_map = &(c->up_frame);
		return TRUE;
	}
	if (x >= 190 && x < 218 && y >= 245 && y < 273) {
		*button_map = &(c->down_frame);
		return TRUE;
	}
	if (x >= 102 && x < 130 && y >= 275 && y < 303){
		*button_map = &(c->broadcast_list_frame);
		return TRUE;
	}
	/* Los 8 buddys */
	if (x >= 18 && x < 188 && y >= 65 && y < 272) {
		g = (y - 65);
		if ((g % 26) < 25) {
			g = g / 26;
			*button_map = &(c->buddys[g]);
		}
	}
	if (y >= 16) {
		/* El evento cae dentro de la ventana */
		return TRUE;
	}
	
	return FALSE;
}

int chat_mouse_motion (Chat *c, int x, int y, int **button_map) {
	int g;
	if (c->ventana.mostrar == FALSE) return FALSE;
	
	/* En caso contrario, buscar si el mouse está en el botón de cierre */
	if (y >= 30 && y < 58 && x >= 190 && x < 218) {
		*button_map = &(c->close_frame);
		return TRUE;
	}
	if (x >= 190 && x < 218 && y >= 61 && y < 89) {
		*button_map = &(c->up_frame);
		return TRUE;
	}
	if (x >= 190 && x < 218 && y >= 245 && y < 273) {
		*button_map = &(c->down_frame);
		return TRUE;
	}
	if (x >= 102 && x < 130 && y >= 275 && y < 303){
		*button_map = &(c->broadcast_list_frame);
		return TRUE;
	}
	/* Los 8 buddys */
	if (x >= 18 && x < 188 && y >= 65 && y < 272) {
		g = (y - 65);
		if ((g % 26) < 25) {
			g = g / 26;
			*button_map = &(c->buddys[g]);
		}
	}
	if ((y >= 16) || (x >= 64 && x < 168)) {
		/* El evento cae dentro de la ventana */
		return TRUE;
	}
	return FALSE;
}

int chat_mouse_up (Chat *c, int x, int y, int **button_map) {
	int g, h;
	BuddyMCast *buddy;
	if (c->ventana.mostrar == FALSE) return FALSE;
	
	/* En caso contrario, buscar si el mouse está en el botón de cierre */
	if (y >= 30 && y < 58 && x >= 190 && x < 218) {
		*button_map = &(c->close_frame);
		if (cp_button_up (*button_map)) {
			c->ventana.mostrar = FALSE;
		}
		return TRUE;
	}
	
	/* Botón flecha hacia arriba */
	if (x >= 190 && x < 218 && y >= 61 && y < 89) {
		*button_map = &(c->up_frame);
		if (cp_button_up (*button_map)) {
			if (c->list_offset > 0) c->list_offset -= 8; /* Asumiendo que siempre son múltiplos de 8 */
		}
		return TRUE;
	}
	/* Botón flecha hacia abajo */
	if (x >= 190 && x < 218 && y >= 245 && y < 273) {
		*button_map = &(c->down_frame);
		if (cp_button_up (*button_map)) {
			if (c->list_display == CHAT_LIST_MCAST && c->list_offset + 8 < c->buddy_mcast_count) {
				c->list_offset += 8;
			}
		}
		return TRUE;
	}
	
	/* Buddy list broadcast local */
	if (x >= 102 && x < 130 && y >= 275 && y < 303){
		*button_map = &(c->broadcast_list_frame);
		if (cp_button_up (*button_map)) {
			if (c->list_display != CHAT_LIST_MCAST) {
				c->list_offset = 0;
				c->list_display = CHAT_LIST_MCAST;
			}
		}
		return TRUE;
	}
	/* Los 8 buddys */
	if (x >= 18 && x < 188 && y >= 65 && y < 272) {
		g = (y - 65);
		if ((g % 26) < 25) {
			g = g / 26;
			*button_map = &(c->buddys[g]);
			if (cp_button_up (*button_map)) {
				/* Calcular el buddy seleccionado */
				h = c->list_offset + g;
				if (c->list_display == CHAT_LIST_MCAST && h < c->buddy_mcast_count) {
					/* Recorrer la lista */
					buddy = c->buddy_mcast;
					
					g = 0;
					while (g < h) {
						buddy = buddy->next;
						g++;
					}
					Juego *j;
					j = crear_juego ();
					conectar_con_sockaddr (j, nick_global, (struct sockaddr *)&buddy->cliente, buddy->tamsock);
				}
				
				/* Revisar otras listas */
			}
		}
	}
	if ((y >= 16) || (x >= 64 && x < 168)) {
		/* El evento cae dentro de la ventana */
		return TRUE;
	}
	return FALSE;
}

void chat_draw (Chat *c, SDL_Surface *screen) {
	SDL_Rect rect;
	int g, h;
	BuddyMCast *buddy;
	
	if (azul1 == 0) {
		azul1 = SDL_MapRGB (screen->format, 0, 0x52, 0x9B);
	}
	
	rect.x = c->ventana.x;
	rect.y = c->ventana.y;
	rect.w = c->ventana.w;
	rect.h = c->ventana.h;
	
	SDL_BlitSurface (images[IMG_WINDOW_CHAT], NULL, screen, &rect);
	
	/* Dibujar el botón de cierre */
	rect.x = c->ventana.x + 190;
	rect.y = c->ventana.y + 30;
	rect.w = images[IMG_BUTTON_CLOSE_UP]->w;
	rect.h = images[IMG_BUTTON_CLOSE_UP]->h;
	
	SDL_BlitSurface (images[c->close_frame], NULL, screen, &rect);
	
	/* Dibujar el color azul de la barra de desplazamiento */
	rect.x = c->ventana.x + 191;
	rect.y = c->ventana.y + 73;
	rect.w = 26;
	rect.h = 188;
	
	SDL_FillRect (screen, &rect, azul1);
	
	/* Botón hacia arriba */
	rect.x = c->ventana.x + 190;
	rect.y = c->ventana.y + 61;
	rect.w = images[IMG_BUTTON_ARROW_1_UP]->w;
	rect.h = images[IMG_BUTTON_ARROW_1_UP]->h;
	
	SDL_BlitSurface (images[c->up_frame], NULL, screen, &rect);
	
	/* Botón hacia abajo */
	rect.x = c->ventana.x + 190;
	rect.y = c->ventana.y + 245;
	rect.w = images[IMG_BUTTON_ARROW_2_UP]->w;
	rect.h = images[IMG_BUTTON_ARROW_2_UP]->h;
	
	SDL_BlitSurface (images[c->down_frame], NULL, screen, &rect);
	
	/* Broadcast list */
	rect.x = c->ventana.x + 102;
	rect.y = c->ventana.y + 275;
	rect.w = images[IMG_BUTTON_LIST_UP]->w;
	rect.h = images[IMG_BUTTON_LIST_UP]->h;
	
	SDL_BlitSurface (images[c->broadcast_list_frame], NULL, screen, &rect);
	
	rect.x = c->ventana.x + 109;
	rect.y = c->ventana.y + 281;
	rect.w = images[IMG_LIST_MINI]->w;
	rect.h = images[IMG_LIST_MINI]->h;
	
	SDL_BlitSurface (images[IMG_LIST_MINI], NULL, screen, &rect);
	
	/* Los 8 buddys */
	for (g = 0; g < 8; g++) {
		rect.x = c->ventana.x + 18;
		rect.y = c->ventana.y + 65 + (g * 26);
		rect.w = images[IMG_SHADOW_UP]->w;
		rect.h = images[IMG_SHADOW_UP]->h;
		
		SDL_BlitSurface (images[c->buddys[g]], NULL, screen, &rect);
	}
	
	/* Desplegar la lista correspondiente */
	if (c->list_display == CHAT_LIST_MCAST) {
		buddy = c->buddy_mcast;
		
		g = 0;
		while (g < c->list_offset && buddy != NULL) {
			buddy = buddy->next;
			g++;
		}
		h = c->ventana.y + 65;
		g = 0;
		while (g < 8 && buddy != NULL) {
			rect.x = c->ventana.x + 26;
			rect.y = h + 3;
			rect.w = images[IMG_LIST_BIG]->w;
			rect.h = images[IMG_LIST_BIG]->h;
			
			SDL_BlitSurface (images[IMG_LIST_BIG], NULL, screen, &rect);
			
			rect.x = c->ventana.x + 46;
			rect.y = h + 5;
			rect.w = buddy->nick_chat->w;
			rect.h = buddy->nick_chat->h;
			SDL_BlitSurface (buddy->nick_chat, NULL, screen, &rect);
			
			h = h + 26;
			g++;
			buddy = buddy->next;
		}
	}
}

void show_chat (void) {
	static_chat->ventana.mostrar = TRUE;
}

void buddy_list_mcast_add (const char *nick, struct sockaddr *direccion, socklen_t tamsock) {
	BuddyMCast *nuevo;
	char buffer[256];
	SDL_Color blanco = {255, 255, 255};
	
	/* Buscar entre los buddys locales que no esté duplicado */
	nuevo = static_chat->buddy_mcast;
	
	while (nuevo != NULL) {
		if (sockaddr_cmp (direccion, (struct sockaddr *) &(nuevo->cliente)) == 0) {
			SDL_FreeSurface (nuevo->nick_chat);
			
			/* Actualizar con el nuevo nombre */
			strncpy (nuevo->nick, nick, NICK_SIZE);
			if (direccion->sa_family == AF_INET) {
				sprintf (buffer, "%s [IPv4]", nick);
			} else if (direccion->sa_family == AF_INET6) {
				if (IN6_IS_ADDR_V4MAPPED (&((struct sockaddr_in6 *)direccion)->sin6_addr)) {
					sprintf (buffer, "%s [IPv4]", nick);
				} else {
					sprintf (buffer, "%s [IPv6]", nick);
				}
			} else {
				sprintf (buffer, "%s [???]", nick);
			}
			nuevo->nick_chat = TTF_RenderUTF8_Blended (ttf14_facefront, buffer, blanco);
			
			return;
		}
		nuevo = nuevo->next;
	}
	
	nuevo = (BuddyMCast *) malloc (sizeof (BuddyMCast));
	
	nuevo->next = static_chat->buddy_mcast;
	static_chat->buddy_mcast = nuevo;
	
	strncpy (nuevo->nick, nick, NICK_SIZE);
	memcpy (&(nuevo->cliente), direccion, tamsock);
	nuevo->tamsock = tamsock;
	
	if (direccion->sa_family == AF_INET) {
		sprintf (buffer, "%s [IPv4]", nick);
	} else if (direccion->sa_family == AF_INET6) {
		if (IN6_IS_ADDR_V4MAPPED (&((struct sockaddr_in6 *)direccion)->sin6_addr)) {
			sprintf (buffer, "%s [IPv4]", nick);
		} else {
			sprintf (buffer, "%s [IPv6]", nick);
		}
	} else {
		sprintf (buffer, "%s [???]", nick);
	}
	nuevo->nick_chat = TTF_RenderUTF8_Blended (ttf14_facefront, buffer, blanco);
	
	printf ("Posible partida de nombre: %s\n", nick);
	static_chat->buddy_mcast_count++;
}

void buddy_list_mcast_remove (struct sockaddr *direccion, socklen_t tamsock) {
	BuddyMCast *this, *prev;
	
	/* Tratar de encontrar el buddy, pero ignorar si no aparece */
	prev = this = static_chat->buddy_mcast;
	
	while (this != NULL) {
		if (sockaddr_cmp (direccion, (struct sockaddr *) &(this->cliente)) == 0) {
			/* Eliminar este item */
			if (this == static_chat->buddy_mcast) {
				static_chat->buddy_mcast = this->next;
			} else {
				prev->next = this->next;
			}
			static_chat->buddy_mcast_count--;
			
			if (static_chat->list_display == CHAT_LIST_MCAST && static_chat->list_offset >= static_chat->buddy_mcast_count) {
				if (static_chat->list_offset > 0) static_chat->list_offset -= 8;
			}
			free (this);
			break;
		}
		this = this->next;
		if (prev != static_chat->buddy_mcast) prev = prev->next;
	}
}
