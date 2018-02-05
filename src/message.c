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
#include "ventana.h"

struct _Message {
	int tipo;
	int button_frame;
	int x, y, w, h;
	SDL_Surface *text_image;
	SDL_Surface *button_text_image;
	
	struct _Message *next;
};

static Message *list_msg = NULL;
static Ventana *msg_window = NULL;

enum {
	MESSAGE_BUTTON_OK,
	
	MESSAGE_NUM_BUTTONS
};

static void message_full_draw (Message *m);
static void message_draw_button (Message *m, int frame);
static int message_mouse_up (Ventana *v, int x, int y);
static int message_mouse_motion (Ventana *v, int x, int y);
static int message_mouse_down (Ventana *v, int x, int y);
static void message_button (Ventana *v, int button);
static void message_button_frame (Ventana *v, int button, int frame);

static void message_drawfuzz (SDL_Surface *surface, int x, int y, int w, int h) {
	int xx, yy;
	SDL_Rect src, dest;

	for (yy = y; yy < y + h; yy = yy + (images[IMG_FUZZ] -> h)) {
		for (xx = x; xx < x + w; xx = xx + (images[IMG_FUZZ] -> w)) {
			src.x = 0;
			src.y = 0;
			src.w = images[IMG_FUZZ] -> w;
			src.h = images[IMG_FUZZ] -> h;

			if (xx + src.w > x + w)
			src.w = x + w - xx;

			if (yy + src.h > y + h)
			src.h = y + h - yy;

			dest.x = xx;
			dest.y = yy;

			dest.w = src.w;
			dest.h = src.h;

			SDL_BlitSurface(images[IMG_FUZZ], &src, surface, &dest);
		}
	}
}

void message_add (int tipo, const char *texto_boton, const char *cadena, ...) {
	Message *new, *end;
	char *msg;
	int size;
	va_list ap;
	SDL_Color blanco;
	int g;
	
	if (msg_window == NULL) {
		msg_window = window_create (760, 480, TRUE);
		
		//window_set_data (msg_window, &list_msg);
		
		window_register_mouse_events (msg_window, message_mouse_down, message_mouse_motion, message_mouse_up);
		window_register_buttons (msg_window, MESSAGE_NUM_BUTTONS, message_button_frame, message_button);
	}
	
	new = (Message *) malloc (sizeof (Message));
	
	if (new == NULL) return;
	
	va_start (ap, cadena);
	
	/* Calcular cuando espacio reservar para generar el mensaje */
	size = vsnprintf (NULL, 0, cadena, ap);
	size = size + 1;
	va_end (ap);
	va_start (ap, cadena);
	msg = (char *) malloc (sizeof (char) * size);
	vsnprintf (msg, size, cadena, ap);
	va_end (ap);
	new->tipo = tipo;
	new->button_frame = 0;
	new->next = NULL;
	
	blanco.r = blanco.g = blanco.b = 0xff;
	new->text_image = draw_text (ttf16_comiccrazy, msg, &blanco);
	
	free (msg);
	
	/* Ancho mínimo 48, más múltiplos de 8 */
	if (new->text_image->w > 152) {
		g = (new->text_image->w / 8) + 3;
		new->w = 40 + (g * 8);
	} else {
		new->w = 216;
	}
	/* Alto mínimo 52, más múltiplos de 8 */
	new->h = 108 + (new->text_image->h / 8) * 8 + 16;
	
	new->x = (760 - new->w) / 2;
	new->y = (480 - new->h) / 2;
	
	if (texto_boton != NULL && texto_boton[0] != 0) {
		new->button_text_image = draw_text (ttf20_comiccrazy, texto_boton, &blanco);
	} else {
		new->button_text_image = NULL;
	}
	
	if (list_msg == NULL) {
		list_msg = new;
		message_full_draw (list_msg);
		window_show (msg_window);
		window_raise (msg_window);
	} else {
		end = list_msg;
		while (end->next != NULL) end = end->next;
		end->next = new;
	}
	
	window_cancel_draging ();
}

static void message_full_draw (Message *m) {
	SDL_Rect rect, rect2, rect3;
	SDL_Surface *surface;
	int p, q, g;
	int ancho, alto;
	int x, y, imagen;
	Uint32 color;
	
	if (m == NULL) return;
	
	surface = window_get_surface (msg_window);
	
	/* Primero, borrar toda la ventana */
	SDL_FillRect (surface, NULL, 0); /* Transparencia total */
	
	if (m->tipo == MSG_ERROR) {
		imagen = IMG_WINDOW_ERROR_SHADOW;
	} else if (m->tipo == MSG_NORMAL) {
		imagen = IMG_WINDOW_PART_SHADOW;
	}
	
	SDL_SetAlpha (images[imagen], 0, 0);
	SDL_SetAlpha (images[IMG_FUZZ], 0, 0);
	
	message_drawfuzz (surface, 0, 0, 760, 480);
	
	p = (m->w - 40) / 8;
	q = (m->h - 36) / 8;
	
	/* Las 4 esquinas */
	rect.x = m->x;
	rect.y = m->y;
	rect.w = 20;
	rect.h = 18;
	
	rect2.x = 0;
	rect2.y = 0;
	rect2.w = 20;
	rect2.h = 18;
	
	SDL_BlitSurface (images[imagen], &rect2, surface, &rect);
	
	rect.x = list_msg->x + list_msg->w - 20;
	rect.y = list_msg->y;
	rect.w = 20;
	rect.h = 18;
	
	rect2.x = 28;
	rect2.y = 0;
	
	SDL_BlitSurface (images[imagen], &rect2, surface, &rect);
	
	rect.x = list_msg->x;
	rect.y = list_msg->y + list_msg->h - 18;
	rect.w = 20;
	rect.h = 18;
	
	rect2.x = 0;
	rect2.y = 26;
	
	SDL_BlitSurface (images[imagen], &rect2, surface, &rect);
	
	rect.x = list_msg->x + list_msg->w - 20;
	rect.y = list_msg->y + list_msg->h - 18;
	rect.w = 20;
	rect.h = 18;
	
	rect2.x = 28;
	rect2.y = 26;
	
	SDL_BlitSurface (images[imagen], &rect2, surface, &rect);
	
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
		
		SDL_BlitSurface (images[imagen], &rect2, surface, &rect);
		
		rect.x = list_msg->x + 20 + (g * 8);
		rect.y = list_msg->y + list_msg->h - 18;
		rect.w = 8;
		rect.h = 18;
		
		SDL_BlitSurface (images[imagen], &rect3, surface, &rect);
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
		
		SDL_BlitSurface (images[imagen], &rect2, surface, &rect);
		
		rect.x = list_msg->x + list_msg->w - 20;
		rect.y = list_msg->y + 18 + (g * 8);
		rect.w = 20;
		rect.h = 8;
		
		SDL_BlitSurface (images[imagen], &rect3, surface, &rect);
	}
	
	/* Rellenar todo el centro de color sólido */
	if (list_msg->tipo == MSG_NORMAL) {
		color = SDL_MapRGB (surface->format, 0x02, 0x80, 0xcd);
	} else if (list_msg->tipo == MSG_ERROR) {
		color = SDL_MapRGB (surface->format, 0xFF, 0x66, 0x00);
	}
	
	rect.x = list_msg->x + 20;
	rect.w = list_msg->w - 40;
	rect.y = list_msg->y + 18;
	rect.h = list_msg->h - 36;
	
	SDL_FillRect (surface, &rect, color);
	
	/* Dibujar el texto */
	rect.w = list_msg->text_image->w;
	rect.h = list_msg->text_image->h;
	rect.x = list_msg->x + (list_msg->w - rect.w) / 2;
	rect.y = list_msg->y + 30;
	
	SDL_BlitSurface (list_msg->text_image, NULL, surface, &rect);
	
	message_draw_button (list_msg, 0);
	
	window_flip (msg_window);
}

static void message_draw_button (Message *m, int frame) {
	SDL_Surface *surface;
	SDL_Rect rect;
	int imagen;
	Uint32 color;
	
	surface = window_get_surface (msg_window);
	
	/* Rellenar todo el centro de color sólido */
	if (m->tipo == MSG_NORMAL) {
		color = SDL_MapRGB (surface->format, 0x02, 0x80, 0xcd);
		imagen = IMG_BUTTON_NORMAL_UP;
	} else if (m->tipo == MSG_ERROR) {
		color = SDL_MapRGB (surface->format, 0xFF, 0x66, 0x00);
		imagen = IMG_BUTTON_ERROR_UP;
	}
	
	/* Dibujar el boton */
	rect.w = images[imagen]->w;
	rect.x = m->x + (m->w - rect.w) / 2;
	rect.y = m->y + m->h - 76;
	rect.h = images[imagen]->h;
	
	window_update (msg_window, &rect);
	SDL_FillRect (surface, &rect, color);
	
	rect.w = images[imagen]->w;
	rect.x = m->x + (m->w - rect.w) / 2;
	rect.y = m->y + m->h - 76;
	rect.h = images[imagen]->h;
	
	SDL_BlitSurface (images[imagen + frame], NULL, surface, &rect);
	
	if (m->button_text_image != NULL) {
		/* Dibujar el texto del botón */
		rect.w = m->button_text_image->w;
		rect.h = m->button_text_image->h;
		rect.x = m->x + (m->w - rect.w) / 2;
		rect.y = m->y + m->h - 58;
	
		SDL_BlitSurface (m->button_text_image, NULL, surface, &rect);
	}
}

static int message_mouse_down (Ventana *v, int x, int y) {
	int b_x, b_y;
	
	if (list_msg == NULL) return FALSE;
	
	b_x = list_msg->x + (list_msg->w - images[IMG_BUTTON_ERROR_UP]->w) / 2;
	b_y = list_msg->y + list_msg->h - 76;
	
	if (x >= b_x && x < b_x + images[IMG_BUTTON_ERROR_UP]->w && y >= b_y && y < b_y + images[IMG_BUTTON_ERROR_UP]->h) {
		window_button_mouse_down (v, MESSAGE_BUTTON_OK);
		return TRUE;
	}/* else if (x >= list_msg->x && x < list_msg->x + list_msg->w && y >= list_msg->y && y < list_msg->y + list_msg->h) {
		return TRUE;
	}*/
	
	return TRUE;
}

static int message_mouse_motion (Ventana *v, int x, int y) {
	int b_x, b_y;
	
	if (list_msg == NULL) return FALSE;
	
	b_x = list_msg->x + (list_msg->w - images[IMG_BUTTON_ERROR_UP]->w) / 2;
	b_y = list_msg->y + list_msg->h - 76;
	
	if (x >= b_x && x < b_x + images[IMG_BUTTON_ERROR_UP]->w && y >= b_y && y < b_y + images[IMG_BUTTON_ERROR_UP]->h) {
		window_button_mouse_motion (v, MESSAGE_BUTTON_OK);
		return TRUE;
	}/* else if (x >= list_msg->x && x < list_msg->x + list_msg->w && y >= list_msg->y && y < list_msg->y + list_msg->h) {
		return TRUE;
	}*/
	
	return TRUE;
}

static int message_mouse_up (Ventana *v, int x, int y) {
	int b_x, b_y;
	
	if (list_msg == NULL) return FALSE;
	
	b_x = list_msg->x + (list_msg->w - images[IMG_BUTTON_ERROR_UP]->w) / 2;
	b_y = list_msg->y + list_msg->h - 76;
	
	if (x >= b_x && x < b_x + images[IMG_BUTTON_ERROR_UP]->w && y >= b_y && y < b_y + images[IMG_BUTTON_ERROR_UP]->h) {
		window_button_mouse_up (v, MESSAGE_BUTTON_OK);
		return TRUE;
	}
	
	/*if (x >= list_msg->x && x < list_msg->x + list_msg->w && y >= list_msg->y && y < list_msg->y + list_msg->h) {
		return TRUE;
	}*/
	
	return TRUE;
}

static void message_button_frame (Ventana *v, int button, int frame) {
	if (list_msg == NULL) return;
	
	if (button == MESSAGE_BUTTON_OK) {
		message_draw_button (list_msg, frame);
	}
}

static void message_button (Ventana *v, int button) {
	Message *next;
	
	if (list_msg == NULL) return;
	
	if (button == MESSAGE_BUTTON_OK) {
		next = list_msg->next;
		SDL_FreeSurface (list_msg->text_image);
		if (list_msg->button_text_image != NULL) SDL_FreeSurface (list_msg->button_text_image);
		free (list_msg);
		list_msg = next;
		
		if (list_msg == NULL) {
			window_hide (msg_window);
		} else {
			message_full_draw (list_msg);
		}
	}
}

