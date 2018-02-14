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
#include "netplay.h"
#include "juego.h"
#include "ventana.h"

enum {
	CHAT_BUTTON_CLOSE = 0,
	
	CHAT_BUTTON_UP,
	CHAT_BUTTON_DOWN,
	
	CHAT_BUTTON_BROADCAST_LIST,
	CHAT_BUTTON_RECENT_LIST,
	
	CHAT_BUTTON_BUDDY_1,
	CHAT_BUTTON_BUDDY_2,
	CHAT_BUTTON_BUDDY_3,
	CHAT_BUTTON_BUDDY_4,
	CHAT_BUTTON_BUDDY_5,
	CHAT_BUTTON_BUDDY_6,
	CHAT_BUTTON_BUDDY_7,
	CHAT_BUTTON_BUDDY_8,
	
	CHAT_NUM_BUTTONS
};

int chat_mouse_down (Ventana *v, int x, int y);
int chat_mouse_motion (Ventana *v, int x, int y);
int chat_mouse_up (Ventana *v, int x, int y);
void chat_full_draw (Chat *c);
void chat_draw_button_close (Ventana *v, int frame);
void chat_button_frame (Ventana *v, int button, int frame);
void chat_button_event (Ventana *v, int button);

static Chat *static_chat = NULL;

void inicializar_chat (void) {
	Chat *c;
	int g;
	SDL_Color color;
	
	c = (Chat *) malloc (sizeof (Chat));
	
	c->ventana = window_create (232, 324, FALSE);
	window_set_data (c->ventana, c);
	window_register_mouse_events (c->ventana, chat_mouse_down, chat_mouse_motion, chat_mouse_up);
	window_register_buttons (c->ventana, CHAT_NUM_BUTTONS, chat_button_frame, chat_button_event);
	
	color.r = color.g = color.b = 0xff;
	
	/* La lista de jugadores multicast locales */
	c->buddy_mcast = NULL;
	c->buddy_mcast_count = 0;
	c->mcast_text = TTF_RenderUTF8_Blended (ttf16_comiccrazy, "Red local", color);
	
	/* La lista de jugadores recientes */
	c->buddy_recent = NULL;
	c->buddy_recent_count = 0;
	c->recent_text = TTF_RenderUTF8_Blended (ttf16_comiccrazy, "Recientes", color);
	
	/* Cuál de las listas se despliega */
	c->list_display = CHAT_LIST_MCAST;
	c->list_offset = 0;
	
	c->azul1 = SDL_MapRGB (window_get_surface (c->ventana)->format, 0, 0x52, 0x9B);
	
	memset (c->buddys_frame, 0, sizeof (c->buddys_frame));
	c->close_frame = 0;
	
	chat_full_draw (c);
	
	static_chat = c;
}

int chat_mouse_down (Ventana *v, int x, int y) {
	int g;
	Chat *c = (Chat *) window_get_data (v);
	
	if (x >= 64 && x < 168 && y < 22) {
		/* Click por el agarre */
		window_start_drag (v, x, y);
		return TRUE;
	}
	if (y >= 30 && y < 58 && x >= 190 && x < 218) {
		/* El click cae en el botón de cierre de la ventana */
		window_button_mouse_down (v, CHAT_BUTTON_CLOSE);
		return TRUE;
	}
	if (x >= 190 && x < 218 && y >= 61 && y < 89) {
		window_button_mouse_down (v, CHAT_BUTTON_UP);
		return TRUE;
	}
	if (x >= 190 && x < 218 && y >= 245 && y < 273) {
		window_button_mouse_down (v, CHAT_BUTTON_DOWN);
		return TRUE;
	}
	if (x >= 102 && x < 130 && y >= 275 && y < 303) {
		window_button_mouse_down (v, CHAT_BUTTON_BROADCAST_LIST);
		return TRUE;
	}
	if (x >= 134 && x < 162 && y >= 275 && y < 303) {
		window_button_mouse_down (v, CHAT_BUTTON_RECENT_LIST);
		return TRUE;
	}
	/* Los 8 buddys */
	if (x >= 18 && x < 188 && y >= 65 && y < 272) {
		g = (y - 65);
		if ((g % 26) < 25) {
			g = g / 26;
			window_button_mouse_down (v, CHAT_BUTTON_BUDDY_1 + g);
		}
		
		return TRUE;
	}
	
	if (y >= 16) {
		/* El evento cae dentro de la ventana */
		return TRUE;
	}
	
	return FALSE;
}

int chat_mouse_motion (Ventana *v, int x, int y) {
	int g;
	Chat *c = (Chat *) window_get_data (v);
	
	/* En caso contrario, buscar si el mouse está en el botón de cierre */
	if (y >= 30 && y < 58 && x >= 190 && x < 218) {
		window_button_mouse_motion (v, CHAT_BUTTON_CLOSE);
		return TRUE;
	}
	if (x >= 190 && x < 218 && y >= 61 && y < 89) {
		window_button_mouse_motion (v, CHAT_BUTTON_UP);
		return TRUE;
	}
	if (x >= 190 && x < 218 && y >= 245 && y < 273) {
		window_button_mouse_motion (v, CHAT_BUTTON_DOWN);
		return TRUE;
	}
	if (x >= 102 && x < 130 && y >= 275 && y < 303){
		window_button_mouse_motion (v, CHAT_BUTTON_BROADCAST_LIST);
		return TRUE;
	}
	if (x >= 134 && x < 162 && y >= 275 && y < 303) {
		window_button_mouse_motion (v, CHAT_BUTTON_RECENT_LIST);
		return TRUE;
	}
	/* Los 8 buddys */
	if (x >= 18 && x < 188 && y >= 65 && y < 272) {
		g = (y - 65);
		if ((g % 26) < 25) {
			g = g / 26;
			window_button_mouse_motion (v, CHAT_BUTTON_BUDDY_1 + g);
		}
		return TRUE;
	}
	if ((y >= 16) || (x >= 64 && x < 168)) {
		/* El evento cae dentro de la ventana */
		return TRUE;
	}
	return FALSE;
}

int chat_mouse_up (Ventana *v, int x, int y) {
	int g;
	
	/* En caso contrario, buscar si el mouse está en el botón de cierre */
	if (y >= 30 && y < 58 && x >= 190 && x < 218) {
		window_button_mouse_up (v, CHAT_BUTTON_CLOSE);
		return TRUE;
	}
	
	/* Botón flecha hacia arriba */
	if (x >= 190 && x < 218 && y >= 61 && y < 89) {
		window_button_mouse_up (v, CHAT_BUTTON_UP);
		return TRUE;
	}
	/* Botón flecha hacia abajo */
	if (x >= 190 && x < 218 && y >= 245 && y < 273) {
		window_button_mouse_up (v, CHAT_BUTTON_DOWN);
		return TRUE;
	}
	
	/* Buddy list broadcast local */
	if (x >= 102 && x < 130 && y >= 275 && y < 303){
		window_button_mouse_up (v, CHAT_BUTTON_BROADCAST_LIST);
		return TRUE;
	}
	
	if (x >= 134 && x < 162 && y >= 275 && y < 303) {
		window_button_mouse_up (v, CHAT_BUTTON_RECENT_LIST);
		return TRUE;
	}
	
	/* Los 8 buddys */
	if (x >= 18 && x < 188 && y >= 65 && y < 272) {
		g = (y - 65);
		if ((g % 26) < 25) {
			g = g / 26;
			window_button_mouse_up (v, CHAT_BUTTON_BUDDY_1 + g);
		}
		return TRUE;
	}
	if ((y >= 16) || (x >= 64 && x < 168)) {
		/* El evento cae dentro de la ventana */
		return TRUE;
	}
	return FALSE;
}

void chat_button_frame (Ventana *v, int button, int frame) {
	SDL_Surface *surface = window_get_surface (v);
	Chat *c = (Chat *) window_get_data (v);
	SDL_Rect rect, rect2;
	int g;
	
	switch (button) {
		case CHAT_BUTTON_CLOSE:
			c->close_frame = frame;
			
			/* Dibujar el botón de cierre */
			rect.x = 190;
			rect.y = 30;
			rect.w = images[IMG_BUTTON_CLOSE_UP]->w;
			rect.h = images[IMG_BUTTON_CLOSE_UP]->h;
			
			window_update (v, &rect);
			SDL_BlitSurface (images[IMG_WINDOW_CHAT], &rect, surface, &rect);
	
			SDL_BlitSurface (images[IMG_BUTTON_CLOSE_UP + frame], NULL, surface, &rect);
			break;
		case CHAT_BUTTON_UP:
			rect.x = 190;
			rect.y = 61;
			rect.w = images[IMG_BUTTON_ARROW_1_UP]->w;
			rect.h = images[IMG_BUTTON_ARROW_1_UP]->h;
			
			window_update (v, &rect);
			SDL_BlitSurface (images[IMG_WINDOW_CHAT], &rect, surface, &rect);
			
			/* Dibujar el color azul de la barra de desplazamiento */
			rect.x = 191;
			rect.y = 73;
			rect.w = 26;
			rect.h = images[IMG_BUTTON_ARROW_1_UP]->h + 2;
			
			SDL_FillRect (surface, &rect, c->azul1);
			
			/* Botón hacia arriba */
			rect.x = 190;
			rect.y = 61;
			rect.w = images[IMG_BUTTON_ARROW_1_UP]->w;
			rect.h = images[IMG_BUTTON_ARROW_1_UP]->h;
			
			SDL_BlitSurface (images[IMG_BUTTON_ARROW_1_UP + frame], NULL, surface, &rect);
			break;
		case CHAT_BUTTON_DOWN:
			rect.x = 190;
			rect.y = 245;
			rect.w = images[IMG_BUTTON_ARROW_2_UP]->w;
			rect.h = images[IMG_BUTTON_ARROW_2_UP]->h;
			
			window_update (v, &rect);
			SDL_BlitSurface (images[IMG_WINDOW_CHAT], &rect, surface, &rect);
			
			/* Dibujar el color azul de la barra de desplazamiento */
			rect.x = 191;
			rect.y = 231;
			rect.w = 26;
			rect.h = 30;
			
			SDL_FillRect (surface, &rect, c->azul1);
			
			rect.x = 190;
			rect.y = 245;
			rect.w = images[IMG_BUTTON_ARROW_2_UP]->w;
			rect.h = images[IMG_BUTTON_ARROW_2_UP]->h;
			
			SDL_BlitSurface (images[IMG_BUTTON_ARROW_2_UP + frame], NULL, surface, &rect);
			break;
		case CHAT_BUTTON_BROADCAST_LIST:
			/* Broadcast list */
			rect.x = 102;
			rect.y = 275;
			rect.w = images[IMG_BUTTON_LIST_UP]->w;
			rect.h = images[IMG_BUTTON_LIST_UP]->h;
			
			window_update (v, &rect);
			SDL_BlitSurface (images[IMG_WINDOW_CHAT], &rect, surface, &rect);
			
			SDL_BlitSurface (images[IMG_BUTTON_LIST_UP + frame], NULL, surface, &rect);
			
			rect.x = 109;
			rect.y = 281;
			rect.w = images[IMG_LIST_MINI]->w;
			rect.h = images[IMG_LIST_MINI]->h;
			
			SDL_BlitSurface (images[IMG_LIST_MINI], NULL, surface, &rect);
			break;
		case CHAT_BUTTON_RECENT_LIST:
			/* Recent list */
			rect.x = 134;
			rect.y = 275;
			rect.w = images[IMG_BUTTON_LIST_UP]->w;
			rect.h = images[IMG_BUTTON_LIST_UP]->h;
			
			window_update (v, &rect);
			SDL_BlitSurface (images[IMG_WINDOW_CHAT], &rect, surface, &rect);
			
			SDL_BlitSurface (images[IMG_BUTTON_LIST_UP + frame], NULL, surface, &rect);
			
			rect.x = 141;
			rect.y = 281;
			rect.w = images[IMG_LIST_MINI]->w;
			rect.h = images[IMG_LIST_MINI]->h;
			
			SDL_BlitSurface (images[IMG_LIST_RECENT_MINI], NULL, surface, &rect);
			break;
		case CHAT_BUTTON_BUDDY_1:
		case CHAT_BUTTON_BUDDY_2:
		case CHAT_BUTTON_BUDDY_3:
		case CHAT_BUTTON_BUDDY_4:
		case CHAT_BUTTON_BUDDY_5:
		case CHAT_BUTTON_BUDDY_6:
		case CHAT_BUTTON_BUDDY_7:
		case CHAT_BUTTON_BUDDY_8:
			{
			int b;
			BuddyMCast *buddy;
			RecentPlay *recent;
			
			g = button - CHAT_BUTTON_BUDDY_1;
			c->buddys_frame[g] = frame;
			
			rect.x = 18;
			rect.y = 65 + (g * 26);
			rect.w = images[IMG_SHADOW_UP]->w;
			rect.h = images[IMG_SHADOW_UP]->h;
			
			window_update (v, &rect);
			SDL_BlitSurface (images[IMG_WINDOW_CHAT], &rect, surface, &rect);
			
			SDL_BlitSurface (images[IMG_SHADOW_UP + frame], NULL, surface, &rect);
			
			/* Dibujar el texto encima del botón */
			if (c->list_display == CHAT_LIST_MCAST) {
				b = 0;
				
				buddy = c->buddy_mcast;
				
				while (b < (c->list_offset + g) && buddy != NULL) {
					buddy = buddy->next;
					b++;
				}
				
				/* Si hay buddy, dibujarlo */
				if (buddy != NULL) {
					rect.x = 26;
					rect.y = 65 + (g * 26) + 3;
					rect.w = images[IMG_LIST_BIG]->w;
					rect.h = images[IMG_LIST_BIG]->h;
		
					SDL_BlitSurface (images[IMG_LIST_BIG], NULL, surface, &rect);
		
					rect.x = 46;
					rect.y = 65 + (g * 26) + 5;
					rect.w = buddy->nick_chat->w;
					rect.h = buddy->nick_chat->h;
					SDL_BlitSurface (buddy->nick_chat, NULL, surface, &rect);
				}
			} else if (c->list_display == CHAT_LIST_RECENT) {
				b = 0;
				
				recent = c->buddy_recent;
				
				while (b < (c->list_offset + g) && recent != NULL) {
					recent = recent->next;
					b++;
				}
				
				if (recent != NULL) {
					rect.x = 26;
					rect.y = 65 + (g * 26) + 3;
					rect.w = images[IMG_LIST_BIG]->w;
					rect.h = images[IMG_LIST_BIG]->h;
					
					SDL_BlitSurface (images[IMG_LIST_BIG], NULL, surface, &rect);
					
					rect2.x = rect2.y = 0;
					rect2.h = recent->text->h;
					rect2.w = (recent->text->w > 142 ? 142 : recent->text->w);
					
					rect.x = 46;
					rect.y = 65 + (g * 26) + 5;
					rect.w = (recent->text->w > 142 ? 142 : recent->text->w);
					rect.h = recent->text->h;
					SDL_BlitSurface (recent->text, &rect2, surface, &rect);
				}
			}
			
			}
			break;
	}
}

void chat_button_event (Ventana *v, int button) {
	Chat *c = (Chat *) window_get_data (v);
	SDL_Rect rect;
	SDL_Surface *surface = window_get_surface (v);
	int g;
	
	switch (button) {
		case CHAT_BUTTON_CLOSE:
			window_hide (v);
			break;
		case CHAT_BUTTON_UP:
			if (c->list_offset > 0) {
				c->list_offset -= 8; /* Asumiendo que siempre son múltiplos de 8 */
				for (g = 0; g < 8; g++) {
					chat_button_frame (static_chat->ventana, CHAT_BUTTON_BUDDY_1 + g, static_chat->buddys_frame[g]);
				}
			}
			break;
		case CHAT_BUTTON_DOWN:
			if (c->list_display == CHAT_LIST_MCAST && c->list_offset + 8 < c->buddy_mcast_count) {
				c->list_offset += 8;
				for (g = 0; g < 8; g++) {
					chat_button_frame (static_chat->ventana, CHAT_BUTTON_BUDDY_1 + g, static_chat->buddys_frame[g]);
				}
			} else if (c->list_display == CHAT_LIST_RECENT && c->list_offset + 8 < c->buddy_recent_count) {
				c->list_offset += 8;
				for (g = 0; g < 8; g++) {
					chat_button_frame (static_chat->ventana, CHAT_BUTTON_BUDDY_1 + g, static_chat->buddys_frame[g]);
				}
			}
			break;
		case CHAT_BUTTON_BROADCAST_LIST:
			if (c->list_display != CHAT_LIST_MCAST) {
				c->list_offset = 0;
				c->list_display = CHAT_LIST_MCAST;
				
				/* Cuando la lista cambia, redibujar todos los buddys y el texto arriba */
				for (g = 0; g < 8; g++) {
					chat_button_frame (static_chat->ventana, CHAT_BUTTON_BUDDY_1 + g, static_chat->buddys_frame[g]);
				}
				
				/* Borrar el texto anterior */
				rect.x = 0;
				rect.w = images[IMG_WINDOW_CHAT]->w;
				rect.y = 36;
				rect.h = 18;
				
				SDL_BlitSurface (images[IMG_WINDOW_CHAT], &rect, surface, &rect);
				window_update (v, &rect);
				
				rect.x = (images[IMG_WINDOW_CHAT]->w - c->mcast_text->w) / 2;
				rect.y = 36;
				rect.w = c->mcast_text->w;
				rect.h = c->mcast_text->h;
				
				SDL_BlitSurface (c->mcast_text, NULL, surface, &rect);
				
				chat_button_frame (static_chat->ventana, CHAT_BUTTON_CLOSE, static_chat->close_frame);
			}
			break;
		case CHAT_BUTTON_RECENT_LIST:
			if (c->list_display != CHAT_LIST_RECENT) {
				c->list_offset = 0;
				c->list_display = CHAT_LIST_RECENT;
				
				/* Cuando la lista cambia, redibujar todos los buddys y el texto arriba */
				for (g = 0; g < 8; g++) {
					chat_button_frame (static_chat->ventana, CHAT_BUTTON_BUDDY_1 + g, static_chat->buddys_frame[g]);
				}
				
				/* Borrar el texto anterior */
				rect.x = 0;
				rect.w = images[IMG_WINDOW_CHAT]->w;
				rect.y = 36;
				rect.h = 18;
				
				SDL_BlitSurface (images[IMG_WINDOW_CHAT], &rect, surface, &rect);
				window_update (v, &rect);
				
				rect.x = (images[IMG_WINDOW_CHAT]->w - c->recent_text->w) / 2;
				rect.y = 36;
				rect.w = c->mcast_text->w;
				rect.h = c->mcast_text->h;
				
				SDL_BlitSurface (c->recent_text, NULL, surface, &rect);
				
				chat_button_frame (static_chat->ventana, CHAT_BUTTON_CLOSE, static_chat->close_frame);
			}
			break;
		case CHAT_BUTTON_BUDDY_1:
		case CHAT_BUTTON_BUDDY_2:
		case CHAT_BUTTON_BUDDY_3:
		case CHAT_BUTTON_BUDDY_4:
		case CHAT_BUTTON_BUDDY_5:
		case CHAT_BUTTON_BUDDY_6:
		case CHAT_BUTTON_BUDDY_7:
		case CHAT_BUTTON_BUDDY_8:
			{
			int g, h;
			Juego *j;
			BuddyMCast *buddy;
			RecentPlay *recent;
			
			/* Calcular el buddy seleccionado */
			g = button - CHAT_BUTTON_BUDDY_1;
			h = c->list_offset + g;
			if (c->list_display == CHAT_LIST_MCAST && h < c->buddy_mcast_count) {
				/* Recorrer la lista */
				buddy = c->buddy_mcast;
				
				g = 0;
				while (g < h) {
					buddy = buddy->next;
					g++;
				}
				j = crear_juego (TRUE);
				conectar_con_sockaddr (j, nick_global, (struct sockaddr *)&buddy->cliente, buddy->tamsock);
			} else if (c->list_display == CHAT_LIST_RECENT && h < c->buddy_recent_count) {
				/* Recorrer la lista */
				recent = c->buddy_recent;
				
				g = 0;
				while (g < h) {
					recent = recent->next;
					g++;
				}
				
				nueva_conexion (NULL, recent->buffer);
			}
			
			/* Revisar otras listas */
			}
			break;
	}
}

void chat_full_draw (Chat *c) {
	SDL_Rect rect, rect2;
	int g, h;
	BuddyMCast *buddy;
	RecentPlay *recent;
	SDL_Surface *surface = window_get_surface (c->ventana);
	
	SDL_SetAlpha (images[IMG_WINDOW_CHAT], 0, 0);
	SDL_BlitSurface (images[IMG_WINDOW_CHAT], NULL, surface, NULL);
	
	/* Dibujar el botón de cierre */
	chat_button_frame (c->ventana, CHAT_BUTTON_CLOSE, 0);
	
	/* Dibujar el color azul de la barra de desplazamiento */
	rect.x = 191;
	rect.y = 73;
	rect.w = 26;
	rect.h = 188;
	
	SDL_FillRect (surface, &rect, c->azul1);
	
	/* Botón hacia arriba/abajo */
	chat_button_frame (c->ventana, CHAT_BUTTON_UP, 0);
	chat_button_frame (c->ventana, CHAT_BUTTON_DOWN, 0);
	
	chat_button_frame (c->ventana, CHAT_BUTTON_BROADCAST_LIST, 0);
	chat_button_frame (c->ventana, CHAT_BUTTON_RECENT_LIST, 0);
		
	/* Los 8 buddys */
	for (g = 0; g < 8; g++) {
		chat_button_frame (c->ventana, CHAT_BUTTON_BUDDY_1 + g, 0);
	}
	
	/* TODO: Cuando la lista cambie, borrar y volver a dibujar */
	if (c->list_display == CHAT_LIST_MCAST) {
		rect.x = (images[IMG_WINDOW_CHAT]->w - c->mcast_text->w) / 2;
		rect.y = 36;
		rect.w = c->mcast_text->w;
		rect.h = c->mcast_text->h;
		
		SDL_BlitSurface (c->mcast_text, NULL, surface, &rect);
	} else if (c->list_display == CHAT_LIST_RECENT) {
		rect.x = (images[IMG_WINDOW_CHAT]->w - c->recent_text->w) / 2;
		rect.y = 36;
		rect.w = c->mcast_text->w;
		rect.h = c->mcast_text->h;
		
		SDL_BlitSurface (c->recent_text, NULL, surface, &rect);
	}
	
	window_flip (c->ventana);
}

void show_chat (void) {
	window_show (static_chat->ventana);
}

void buddy_list_mcast_add (const char *nick, struct sockaddr *direccion, socklen_t tamsock) {
	BuddyMCast *nuevo;
	char buffer[256];
	SDL_Color blanco = {255, 255, 255};
	int g;
	
	/* Buscar entre los buddys locales que no esté duplicado */
	nuevo = static_chat->buddy_mcast;
	
	while (nuevo != NULL) {
		if (sockaddr_cmp (direccion, (struct sockaddr *) &(nuevo->cliente)) == 0) {
			/* Actualizar el "last_seen" */
			nuevo->last_seen = SDL_GetTicks();
			
			if (strcmp (nuevo->nick, nick) == 0) return;
			
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
			SDL_FreeSurface (nuevo->nick_chat);
			nuevo->nick_chat = TTF_RenderUTF8_Blended (ttf14_facefront, buffer, blanco);
			
			/* Programar el redibujado de los nicks */
			if (static_chat->list_display == CHAT_LIST_MCAST) {
				for (g = 0; g < 8; g++) {
					chat_button_frame (static_chat->ventana, CHAT_BUTTON_BUDDY_1 + g, static_chat->buddys_frame[g]);
				}
			}
			
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
	nuevo->last_seen = SDL_GetTicks();
	
	static_chat->buddy_mcast_count++;
	
	/* Programar el redibujado de los buddys */
	if (static_chat->list_display == CHAT_LIST_MCAST) {
		for (g = 0; g < 8; g++) {
			chat_button_frame (static_chat->ventana, CHAT_BUTTON_BUDDY_1 + g, static_chat->buddys_frame[g]);
		}
	}
}

void buddy_list_mcast_remove_item (BuddyMCast *item) {
	BuddyMCast *prev;
	int g;
	
	if (item == static_chat->buddy_mcast) {
		/* Principio de la lista */
		static_chat->buddy_mcast = item->next;
	} else {
		prev = static_chat->buddy_mcast;
		
		while (prev->next != item) prev = prev->next;
		
		prev->next = item->next;
	}
	static_chat->buddy_mcast_count--;
	
	if (static_chat->list_display == CHAT_LIST_MCAST && static_chat->list_offset >= static_chat->buddy_mcast_count) {
		if (static_chat->list_offset > 0) static_chat->list_offset -= 8;
	}
	
	free (item);
	
	if (static_chat->list_display == CHAT_LIST_MCAST) {
		/* Programar el redibujado de los buddys */
		for (g = 0; g < 8; g++) {
			chat_button_frame (static_chat->ventana, CHAT_BUTTON_BUDDY_1 + g, static_chat->buddys_frame[g]);
		}
	}
}

void buddy_list_mcast_remove (struct sockaddr *direccion, socklen_t tamsock) {
	BuddyMCast *this;
	int g;
	
	/* Tratar de encontrar el buddy, pero ignorar si no aparece */
	this = static_chat->buddy_mcast;
	
	while (this != NULL) {
		if (sockaddr_cmp (direccion, (struct sockaddr *) &(this->cliente)) == 0) {
			buddy_list_mcast_remove_item (this);
			if (static_chat->list_display == CHAT_LIST_MCAST) {
				/* Programar el redibujado de los buddys */
				for (g = 0; g < 8; g++) {
					chat_button_frame (static_chat->ventana, CHAT_BUTTON_BUDDY_1 + g, static_chat->buddys_frame[g]);
				}
			}
			break;
		}
		this = this->next;
	}
}

void buddy_list_mcast_clean (Uint32 timestamp) {
	BuddyMCast *this, *next;
	
	/* Recorrer todos los buddys */
	this = static_chat->buddy_mcast;
	
	while (this != NULL) {
		/* Si el timestamp supera a los 40 segundos, borrar el buddy */
		if (timestamp > this->last_seen + NET_MCAST_TIMEOUT) {
			/* Eliminar este item */
			next = this->next;
			buddy_list_mcast_remove_item (this);
			
			this = next;
			continue;
		}
		this = this->next;
	}
}

void buddy_list_recent_add (const char *texto) {
	RecentPlay *nuevo;
	char buffer[256];
	SDL_Color blanco = {255, 255, 255};
	int g;
	
	/* Buscar entre los buddys locales que no esté duplicado */
	nuevo = static_chat->buddy_recent;
	
	while (nuevo != NULL) {
		if (strcmp (texto, nuevo->buffer) == 0) return;
		
		nuevo = nuevo->next;
	}
	
	nuevo = (RecentPlay *) malloc (sizeof (RecentPlay));
	
	nuevo->next = static_chat->buddy_recent;
	static_chat->buddy_recent = nuevo;
	
	strncpy (nuevo->buffer, texto, sizeof (nuevo->buffer));
	
	nuevo->text = TTF_RenderUTF8_Blended (ttf14_facefront, texto, blanco);
	static_chat->buddy_recent_count++;
	
	/* Programar el redibujado de los buddys */
	if (static_chat->list_display == CHAT_LIST_RECENT) {
		for (g = 0; g < 8; g++) {
			chat_button_frame (static_chat->ventana, CHAT_BUTTON_BUDDY_1 + g, static_chat->buddys_frame[g]);
		}
	}
}

