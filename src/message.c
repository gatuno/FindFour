/*
 * message.c
 * This file is part of Find Four
 *
 * Copyright (C) 2016 - Félix Arreola Rodríguez
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
#include <stdarg.h>

#include <SDL.h>

#include "findfour.h"
#include "message.h"
#include "draw-text.h"

typedef struct _Message {
	char msg[1024];
	int tipo;
	struct _Message *next;
	int button_frame;
	int x, y, w, h;
	SDL_Surface *text_image;
} Message;

Message *list_msg = NULL;

void message_add (int tipo, const char *cadena, ...) {
	Message *new, *end;
	va_list ap;
	SDL_Color blanco;
	int g;
	
	new = (Message *) malloc (sizeof (Message));
	
	if (new == NULL) return;
	
	va_start (ap, cadena);
	vsprintf (new->msg, cadena, ap);
	new->tipo = tipo;
	new->button_frame = 0;
	new->next = NULL;
	
	blanco.r = blanco.g = blanco.b = 0xff;
	new->text_image = draw_text (ttf16_comiccrazy, new->msg, &blanco);
	
	/* Ancho mínimo 48, más múltiplos de 8 */
	if (new->text_image->w > 216) {
		g = (new->text_image->w / 8) + 3;
		new->w = 40 + (g * 8);
	} else {
		new->w = 216;
	}
	/* Alto mínimo 52, más múltiplos de 8 */
	new->h = 108 + (new->text_image->h / 8) * 8 + 16;
	
	new->x = (760 - new->w) / 2;
	new->y = (480 - new->h) / 2;
	
	if (list_msg == NULL) {
		list_msg = new;
	} else {
		end = list_msg;
		while (end->next != NULL) end = end->next;
		end->next = new;
	}
	
	va_end (ap);
	
	full_stop_drag();
}

void message_display (SDL_Surface *screen) {
	SDL_Rect rect, rect2, rect3;
	int p, q, g;
	int ancho, alto;
	int x, y, imagen;
	Uint32 color;
	
	if (list_msg == NULL) return;
	
	if (list_msg->tipo == MSG_ERROR) {
		imagen = IMG_WINDOW_ERROR;
	} else if (list_msg->tipo == MSG_NORMAL) {
		imagen = IMG_WINDOW_PART;
	}
	
	p = (list_msg->w - 40) / 8;
	q = (list_msg->h - 36) / 8;
	
	/* Las 4 esquinas */
	rect.x = list_msg->x;
	rect.y = list_msg->y;
	rect.w = 20;
	rect.h = 18;
	
	rect2.x = 0;
	rect2.y = 0;
	rect2.w = 20;
	rect2.h = 18;
	
	SDL_BlitSurface (images[imagen], &rect2, screen, &rect);
	
	rect.x = list_msg->x + list_msg->w - 20;
	rect.y = list_msg->y;
	rect.w = 20;
	rect.h = 18;
	
	rect2.x = 28;
	rect2.y = 0;
	
	SDL_BlitSurface (images[imagen], &rect2, screen, &rect);
	
	rect.x = list_msg->x;
	rect.y = list_msg->y + list_msg->h - 18;
	rect.w = 20;
	rect.h = 18;
	
	rect2.x = 0;
	rect2.y = 26;
	
	SDL_BlitSurface (images[imagen], &rect2, screen, &rect);
	
	rect.x = list_msg->x + list_msg->w - 20;
	rect.y = list_msg->y + list_msg->h - 18;
	rect.w = 20;
	rect.h = 18;
	
	rect2.x = 28;
	rect2.y = 26;
	
	SDL_BlitSurface (images[imagen], &rect2, screen, &rect);
	
	/* Parte superior e inferior */
	rect2.w = 8;
	rect2.h = 18;
	rect2.x = 20;
	rect2.y = 0;
	
	rect3.w = 8;
	rect3.h = 18;
	rect3.x = 20;
	rect3.y = 26;
	
	for (g = 0; g < p; g++) {
		rect.x = list_msg->x + 20 + (g * 8);
		rect.y = list_msg->y;
		rect.w = 8;
		rect.h = 18;
		
		SDL_BlitSurface (images[imagen], &rect2, screen, &rect);
		
		rect.x = list_msg->x + 20 + (g * 8);
		rect.y = list_msg->y + list_msg->h - 18;
		rect.w = 8;
		rect.h = 18;
		
		SDL_BlitSurface (images[imagen], &rect3, screen, &rect);
	}
	
	/* Lados derechos e izquierdos */
	rect2.w = 20;
	rect2.h = 8;
	rect2.x = 0;
	rect2.y = 18;
	
	rect3.w = 20;
	rect3.h = 8;
	rect3.x = 28;
	rect3.y = 18;
	
	for (g = 0; g < q; g++) {
		rect.x = list_msg->x;
		rect.y = list_msg->y + 18 + (g * 8);
		rect.w = 20;
		rect.h = 8;
		
		SDL_BlitSurface (images[imagen], &rect2, screen, &rect);
		
		rect.x = list_msg->x + list_msg->w - 20;
		rect.y = list_msg->y + 18 + (g * 8);
		rect.w = 20;
		rect.h = 8;
		
		SDL_BlitSurface (images[imagen], &rect3, screen, &rect);
	}
	
	/* Rellenar todo el centro de color sólido */
	if (list_msg->tipo == MSG_NORMAL) {
		color = SDL_MapRGB (screen->format, 0x02, 0x80, 0xcd);
		imagen = IMG_BUTTON_NORMAL_UP;
	} else if (list_msg->tipo == MSG_ERROR) {
		color = SDL_MapRGB (screen->format, 0xFF, 0x66, 0x00);
		imagen = IMG_BUTTON_ERROR_UP;
	}
	
	rect.x = list_msg->x + 20;
	rect.w = list_msg->w - 40;
	rect.y = list_msg->y + 18;
	rect.h = list_msg->h - 36;
	
	SDL_FillRect (screen, &rect, color);
	
	/* Dibujar el boton */
	rect.w = images[imagen]->w;
	rect.x = list_msg->x + (list_msg->w - rect.w) / 2;
	rect.y = list_msg->y + list_msg->h - 76;
	rect.h = images[imagen]->h;
	
	SDL_BlitSurface (images[imagen + list_msg->button_frame], NULL, screen, &rect);
	
	/* Dibujar el texto */
	rect.w = list_msg->text_image->w;
	rect.h = list_msg->text_image->h;
	rect.x = list_msg->x + (list_msg->w - rect.w) / 2;
	rect.y = list_msg->y + 30;
	
	SDL_BlitSurface (list_msg->text_image, NULL, screen, &rect);
}

int message_mouse_down (int m_x, int m_y, int **button_map) {
	int b_x, b_y;
	
	if (list_msg == NULL) return FALSE;
	
	b_x = list_msg->x + (list_msg->w - images[IMG_BUTTON_ERROR_UP]->w) / 2;
	b_y = list_msg->y + list_msg->h - 76;
	
	if (m_x >= b_x && m_x < b_x + images[IMG_BUTTON_ERROR_UP]->w && m_y >= b_y && m_y < b_y + images[IMG_BUTTON_ERROR_UP]->h) {
		*button_map = &(list_msg->button_frame);
		return TRUE;
	} else if (m_x >= list_msg->x && m_x < list_msg->x + list_msg->w && m_y >= list_msg->y && m_y < list_msg->y + list_msg->h) {
		return TRUE;
	}
	
	return FALSE;
}

int message_mouse_motion (int m_x, int m_y, int **button_map) {
	int b_x, b_y;
	
	if (list_msg == NULL) return FALSE;
	
	b_x = list_msg->x + (list_msg->w - images[IMG_BUTTON_ERROR_UP]->w) / 2;
	b_y = list_msg->y + list_msg->h - 76;
	
	if (m_x >= b_x && m_x < b_x + images[IMG_BUTTON_ERROR_UP]->w && m_y >= b_y && m_y < b_y + images[IMG_BUTTON_ERROR_UP]->h) {
		*button_map = &(list_msg->button_frame);
		return TRUE;
	} else if (m_x >= list_msg->x && m_x < list_msg->x + list_msg->w && m_y >= list_msg->y && m_y < list_msg->y + list_msg->h) {
		return TRUE;
	}
	
	return FALSE;
}

int message_mouse_up (int m_x, int m_y, int **button_map) {
	int b_x, b_y;
	Message *next;
	
	if (list_msg == NULL) return FALSE;
	
	b_x = list_msg->x + (list_msg->w - images[IMG_BUTTON_ERROR_UP]->w) / 2;
	b_y = list_msg->y + list_msg->h - 76;
	
	if (m_x >= b_x && m_x < b_x + images[IMG_BUTTON_ERROR_UP]->w && m_y >= b_y && m_y < b_y + images[IMG_BUTTON_ERROR_UP]->h) {
		*button_map = &(list_msg->button_frame);
		if (cp_button_up (*button_map)) {
			/* Eliminar este mensaje */
			next = list_msg->next;
			SDL_FreeSurface (list_msg->text_image);
			free (list_msg);
			list_msg = next;
		}
		return TRUE;
	}
	
	if (m_x >= list_msg->x && m_x < list_msg->x + list_msg->w && m_y >= list_msg->y && m_y < list_msg->y + list_msg->h) {
		return TRUE;
	}
	
	return FALSE;
}

