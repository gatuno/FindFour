/*
 * inputbox.c
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

#include "utf8.h"

#include "findfour.h"
#include "inputbox.h"
#include "cp-button.h"
#include "ventana.h"

int inputbox_mouse_down (Ventana *v, int x, int y);
int inputbox_mouse_motion (Ventana *v, int x, int y);
int inputbox_mouse_up (Ventana *v, int x, int y);
int inputbox_key_down (Ventana *v, SDL_KeyboardEvent *);

static void draw_inputbox_textfield (InputBox *ib);

enum {
	INPUTBOX_BUTTON_CLOSE = 0,
	INPUTBOX_BUTTON_SEND,
	
	INPUTBOX_NUM_BUTTONS
};

static void full_inputbox_draw (InputBox *ib) {
	SDL_Surface *surface;
	
	SDL_Rect rect, rect2, rect3;
	int p, q, g;
	
	surface = window_get_surface (ib->ventana);
	
	SDL_FillRect (surface, NULL, 0); /* Transparencia total */
	
	SDL_SetAlpha (images[IMG_WINDOW_PART], 0, 0);
	
	p = (ib->w - 144) / 16;
	q = (ib->h - 52) / 8;
	/* Dibujar toda la ventana */
	
	/* Las 4 esquinas */
	rect.x = 0;
	rect.y = 16;
	rect.w = 20;
	rect.h = 18;
	
	rect2.x = 0;
	rect2.y = 0;
	rect2.w = 20;
	rect2.h = 18;
	
	SDL_BlitSurface (images[IMG_WINDOW_PART], &rect2, surface, &rect);
	
	rect.x = ib->w - 20;
	rect.y = 16;
	rect.w = 20;
	rect.h = 18;
	
	rect2.x = 28;
	rect2.y = 0;
	
	SDL_BlitSurface (images[IMG_WINDOW_PART], &rect2, surface, &rect);
	
	rect.x = 0;
	rect.y = ib->h - 18;
	rect.w = 20;
	rect.h = 18;
	
	rect2.x = 0;
	rect2.y = 26;
	
	SDL_BlitSurface (images[IMG_WINDOW_PART], &rect2, surface, &rect);
	
	rect.x = ib->w - 20;
	rect.y = ib->h - 18;
	rect.w = 20;
	rect.h = 18;
	
	rect2.x = 28;
	rect2.y = 26;
	
	SDL_BlitSurface (images[IMG_WINDOW_PART], &rect2, surface, &rect);
	
	/* Parte superior */
	rect2.w = 8;
	rect2.h = 18;
	rect2.x = 20;
	rect2.y = 0;
	
	for (g = 0; g < p; g++) {
		rect.x = 20 + (g * 8);
		rect.y = 16;
		rect.w = 8;
		rect.h = 18;
		
		SDL_BlitSurface (images[IMG_WINDOW_PART], &rect2, surface, &rect);
		
		rect.x = 20 + (g * 8) + 104 + (p * 8);
		rect.y = 16;
		rect.w = 8;
		rect.h = 18;
		
		SDL_BlitSurface (images[IMG_WINDOW_PART], &rect2, surface, &rect);
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
		rect.x = 0;
		rect.y = 34 + (g * 8);
		rect.w = 20;
		rect.h = 8;
		
		SDL_BlitSurface (images[IMG_WINDOW_PART], &rect2, surface, &rect);
		
		rect.x = ib->w - 20;
		rect.y = 34 + (g * 8);
		rect.w = 20;
		rect.h = 8;
		
		SDL_BlitSurface (images[IMG_WINDOW_PART], &rect3, surface, &rect);
	}
	
	/* Lado inferior */
	rect2.x = 20;
	rect2.y = 26;
	rect2.w = 8;
	rect2.h = 18;
	for (g = 0; g < p + p + 13; g++) {
		rect.x = 20 + (g * 8);
		rect.y = ib->h - 18;
		rect.w = 8;
		rect.h = 18;
		
		SDL_BlitSurface (images[IMG_WINDOW_PART], &rect2, surface, &rect);
	}
	
	/* Dibujar el agarre */
	rect.x = 20 + (p * 8);
	rect.y = 0;
	rect.w = images[IMG_WINDOW_TAB]->w;
	rect.h = images[IMG_WINDOW_TAB]->h;
	SDL_SetAlpha (images[IMG_WINDOW_TAB], 0, 0);
	SDL_BlitSurface (images[IMG_WINDOW_TAB], NULL, surface, &rect);
	
	/* Rellenar todo el centro de color sólido */
	rect.x = 20;
	rect.w = ib->w - 40;
	rect.y = 34;
	rect.h = ib->h - 52;
	
	SDL_FillRect (surface, &rect, ib->color_azul);
	
	/* Dibujar la pregunta de texto */
	rect.x = 26;
	rect.y = 42;
	rect.w = ib->text_ask->w;
	rect.h = ib->text_ask->h;
	
	SDL_BlitSurface (ib->text_ask, NULL, surface, &rect);
	
	/* Dibujar el botón de cierre */
	rect.x = ib->w - 42;
	rect.y = 30;
	rect.w = images[IMG_BUTTON_CLOSE_UP]->w;
	rect.h = images[IMG_BUTTON_CLOSE_UP]->h;
	
	SDL_BlitSurface (images[ib->close_frame], NULL, surface, &rect);
	
	draw_inputbox_textfield (ib);
	
	window_flip (ib->ventana);
}

static void draw_inputbox_textfield (InputBox *ib) {
	SDL_Surface *surface;
	
	SDL_Rect rect, rect2, rect3;
	int p, q, g;
	int cursor;
	
	surface = window_get_surface (ib->ventana);
	
	q = (ib->h - 52) / 8;
	
	/* Ahora dibujar el inputbox */
	p = (ib->w - 68) / 10;
	
	/* Borrar antes */
	rect.x = 22;
	rect.y = ib->box_y;
	rect.w = 14 + (10 * p);
	rect.h = 27;
	
	SDL_FillRect (surface, &rect, ib->color_azul);
	
	rect.x = 22 + (p * 10);
	rect.y = ib->box_y;
	rect.w = images[IMG_BUTTON_LIST_UP]->w;
	rect.h = images[IMG_BUTTON_LIST_UP]->h;
	
	SDL_FillRect (surface, &rect, ib->color_azul);
	
	/* Dibujar la caja */
	rect.x = 22;
	rect.y = ib->box_y;
	rect.w = 14;
	rect.h = 27;
	
	rect2.x = rect2.y = 0;
	rect2.w = 14;
	rect2.h = 27;
	
	SDL_BlitSurface (images[IMG_INPUT_BOX], &rect2, surface, &rect);
	
	rect2.x = 14;
	rect2.y = 0;
	rect2.w = 10;
	rect2.h = 27;
	
	for (g = 0; g < p; g++) {
		rect.x = 36 + (g * 10);
		rect.y = ib->box_y;
		rect.w = 10;
		rect.h = 27;
		
		SDL_BlitSurface (images[IMG_INPUT_BOX], &rect2, surface, &rect);
	}
	
	/* Botón de enviar */
	rect.x = 22 + (p * 10);
	rect.y = ib->box_y;
	rect.w = images[IMG_BUTTON_LIST_UP]->w;
	rect.h = images[IMG_BUTTON_LIST_UP]->h;
	
	SDL_BlitSurface (images[ib->send_frame], NULL, surface, &rect);
	
	rect.x = 30 + (p * 10);
	rect.y = ib->box_y + 7;
	rect.w = images[IMG_BUTTON_LIST_UP]->w;
	rect.h = images[IMG_BUTTON_LIST_UP]->h;
	
	SDL_BlitSurface (images[IMG_CHAT_MINI], NULL, surface, &rect);
	
	cursor = 0;
	/* Dibujar la cadena de texto */
	if (ib->text_s != NULL) {
		rect.x = 36;
		rect.y = ib->box_y + 8;
		rect.h = ib->text_s->h;
		
		rect2.y = 0;
		rect2.h = ib->text_s->h;
		
		if (ib->text_s->w > (p * 10)- 16) {
			rect2.w = (p * 10) - 16;
			rect2.x = ib->text_s->w - rect2.w;
		} else {
			rect2.w = ib->text_s->w;
			rect2.x = 0;
		}
		rect.w = rect2.w;
		SDL_BlitSurface (ib->text_s, &rect2, surface, &rect);
		
		cursor += rect.w;
	}
#if 0
	// Pendiente, necesito saber si tengo el foco
	if (get_first_window () == (Ventana *)ib) {
		/* Dibujar el cursor si soy la primera ventana */
		if (ib->cursor_vel < INPUTBOX_CURSOR_RATE) {
			rect.x = 37 + cursor;
			rect.y = ib->box_y + 5;
			rect.w = 1;
			rect.h = 19;

			color = SDL_MapRGB (screen->format, 0xFF, 0xFF, 0xFF);

			SDL_FillRect (screen, &rect, color);
		}
		
		ib->cursor_vel++;
		if (ib->cursor_vel >= 24) ib->cursor_vel = 0;
	} else {
		ib->cursor_vel = 0;
	}
#endif
	q = (ib->h - 52) / 8;
	
	/* Ahora dibujar el inputbox */
	p = (ib->w - 68) / 10;
	
	/* Borrar antes */
	rect.x = 22;
	rect.y = ib->box_y;
	rect.w = 28 + (10 * p);
	rect.h = 27;
	
	window_update (ib->ventana, &rect);
}

static void draw_inputbox_close (InputBox *ib) {
	/* Refrescar solamente el botón de cierre */
	SDL_Surface *surface;
	SDL_Rect rect;
	
	surface = window_get_surface (ib->ventana);
	
	/* Dibujar el botón de cierre */
	rect.x = ib->w - 42;
	rect.y = 30;
	rect.w = images[IMG_BUTTON_CLOSE_UP]->w;
	rect.h = images[IMG_BUTTON_CLOSE_UP]->h;
	
	/* Borrar con fondo primero */
	SDL_FillRect (surface, &rect, ib->color_azul);
	
	SDL_BlitSurface (images[ib->close_frame], NULL, surface, &rect);
	window_update (ib->ventana, &rect);
}

/* Se dispara cuando un botón necesita ser re-dibujado */
void inputbox_button_frame (Ventana *v, int button, int frame) {
	InputBox *ib = (InputBox *) window_get_data (v);
	if (button == INPUTBOX_BUTTON_CLOSE) {
		ib->close_frame = IMG_BUTTON_CLOSE_UP + frame;
		draw_inputbox_close (ib);
	} else if (button == INPUTBOX_BUTTON_SEND) {
		ib->send_frame = IMG_BUTTON_LIST_UP + frame;
		draw_inputbox_textfield (ib);
	}
}

/* Se dispara cuando ocurre un evento de botón en la ventana */
void inputbox_button (Ventana *v, int button) {
	InputBox *ib = (InputBox *) window_get_data (v);
	if (button == INPUTBOX_BUTTON_CLOSE) {
		if (ib->close_callback != NULL) {
			ib->close_callback (ib, ib->buffer);
		}
		eliminar_inputbox (ib);
	} else if (button == INPUTBOX_BUTTON_SEND) {
		if (ib->callback != NULL) {
			ib->callback (ib, ib->buffer);
		}
	}
}

InputBox *crear_inputbox (InputBoxFunc func, const char *ask, const char *text, InputBoxFunc close_func) {
	InputBox *ib;
	SDL_Color blanco;
	blanco.r = blanco.g = blanco.b = 255;
	
	ib = (InputBox *) malloc (sizeof (InputBox));
	
	ib->send_frame = IMG_BUTTON_LIST_UP;
	ib->close_frame = IMG_BUTTON_CLOSE_UP;
	ib->box_y = 80;
	memset (ib->buffer, 0, sizeof (ib->buffer));
	strncpy (ib->buffer, text, sizeof (ib->buffer));
	
	if (strcmp (ib->buffer, "") != 0) {
		ib->text_s = TTF_RenderUTF8_Blended (ttf16_burbank_medium, ib->buffer, blanco);
	} else {
		ib->text_s = NULL;
	}
	
	if (strcmp (ask, "") == 0) {
		ib->text_ask = TTF_RenderUTF8_Blended (ttf16_burbank_medium, "(Sin texto)", blanco);
	} else {
		ib->text_ask = TTF_RenderUTF8_Blended (ttf16_burbank_medium, ask, blanco);
	}
	
	/* Ancho mínimo 144, más múltiplos de 16 */
	/* Alto mínimo 52, más múltiplos de 8 */
	ib->w = 144 + ((ib->text_ask->w / 16) + 1) * 16;
	ib->h = 132;
	ib->ventana = window_create (ib->w, ib->h, TRUE);
	
	window_set_data (ib->ventana, ib);
	
	window_register_mouse_events (ib->ventana, inputbox_mouse_down, inputbox_mouse_motion, inputbox_mouse_up);
	window_register_keyboard_events (ib->ventana, inputbox_key_down, NULL);
	window_register_buttons (ib->ventana, INPUTBOX_NUM_BUTTONS, inputbox_button_frame, inputbox_button);
	
	/* Callbacks de la entrada */
	ib->callback = func;
	ib->close_callback = close_func;
	
	ib->cursor_vel = 0;
	
	ib->color_azul = SDL_MapRGB (window_get_surface(ib->ventana)->format, 0x02, 0x80, 0xcd);
	
	full_inputbox_draw (ib);
	
	return ib;
}

void eliminar_inputbox (InputBox *ib) {
	/* Desligar completamente */
	window_destroy (ib->ventana);
	ib->ventana = NULL;
	
	/* Si hay algún indicativo a estos viejos botones, eliminarlo */
	if (cp_old_map == &(ib->close_frame) || cp_old_map == &(ib->send_frame)) {
		cp_old_map = NULL;
	}
	if (cp_last_button == &(ib->send_frame) || cp_last_button == &(ib->close_frame)) {
		cp_last_button = NULL;
	}
	
	if (ib->text_s != NULL) SDL_FreeSurface (ib->text_s);
	SDL_FreeSurface (ib->text_ask);
	
	free (ib);
}

int inputbox_mouse_down (Ventana *v, int x, int y) {
	int p, q;
	
	InputBox *ib = (InputBox *) window_get_data (v);
	/* El click por el agarre se calcula */
	p = (ib->w - 104) / 2;
	q = ib->w - 68;
	if (x >= p && x < p + 104 && y < 22) {
		/* Click por el agarre */
		window_start_drag (v, x, y);
		return TRUE;
	} else if (x >= 22 + q && x < 50 + q && y >= ib->box_y && y < ib->box_y + 28) {
		window_button_mouse_down (v, INPUTBOX_BUTTON_SEND);
		return TRUE;
	} else if (y >= 30 && y < 58 && x >= ib->w - 42 && x < ib->w - 14) {
		window_button_mouse_down (v, INPUTBOX_BUTTON_CLOSE);
		return TRUE;
	} else if (y >= 16) {
		/* El evento cae dentro de la ventana */
		return TRUE;
	}
	
	return FALSE;
}

int inputbox_mouse_motion (Ventana *v, int x, int y) {
	int p, q;
	
	InputBox *ib = (InputBox *) window_get_data (v);
	
	p = (ib->w - 104) / 2;
	q = ib->w - 68;
	
	if (y >= 30 && y < 58 && x >= ib->w - 42 && x < ib->w - 14) {
		window_button_mouse_motion (v, INPUTBOX_BUTTON_CLOSE);
		return TRUE;
	}
	
	if (x >= 22 + q && x < 50 + q && y >= ib->box_y && y < ib->box_y + 28) {
		window_button_mouse_motion (v, INPUTBOX_BUTTON_SEND);
		return TRUE;
	}
	
	if ((y >= 16) || (x >= p && x < p + 104)) {
		/* El evento cae dentro de la ventana */
		return TRUE;
	}
	
	return FALSE;
}

int inputbox_mouse_up (Ventana *v, int x, int y) {
	int p, q;
	
	InputBox *ib = (InputBox *) window_get_data (v);
	
	/* El click por el agarre se calcula */
	p = (ib->w - 104) / 2;
	q = ib->w - 68;
	
	if (y >= 30 && y < 58 && x >= ib->w - 42 && x < ib->w - 14) {
		window_button_mouse_up (v, INPUTBOX_BUTTON_CLOSE);
		return TRUE;
	}
	
	if (x >= 22 + q && x < 50 + q && y >= ib->box_y && y < ib->box_y + 28) {
		window_button_mouse_up (v, INPUTBOX_BUTTON_SEND);
		return TRUE;
	}
	
	if (y >= 16 || (x >= p && x < p + 104)) {
		/* El evento cae dentro de la ventana */
		return TRUE;
	}
	
	return FALSE;
}

int inputbox_key_down (Ventana *v, SDL_KeyboardEvent *key) {
	int len, g;
	int bytes, bytes2;
	SDL_Color blanco;
	InputBox *ib = (InputBox *) window_get_data (v);
	
	if (key->keysym.unicode == 0) return FALSE;
	
	if (key->keysym.unicode == 13) {
		if (ib->callback != NULL) {
			ib->callback (ib, ib->buffer);
		}
		return TRUE;
	}
	
	if (key->keysym.sym != SDLK_BACKSPACE && (key->keysym.unicode < 32 || key->keysym.unicode == 127)) return TRUE; /* Tecla de eliminar */
	
	if (key->keysym.sym == SDLK_BACKSPACE) {
		/* Eliminar el último caracter */
		len = u8_strlen (ib->buffer);
		if (len == 0) return TRUE;
		
		bytes = u8_offset (ib->buffer, len - 1);
		bytes2 = u8_offset (ib->buffer, len);
		for (g = bytes; g < bytes2; g++) {
			ib->buffer[g] = 0;
		}
	} else {
		len = u8_strlen (ib->buffer);
		bytes = u8_offset (ib->buffer, len);
		
		if (bytes + 4 > sizeof (ib->buffer)) return FALSE; /* Fuera de longitud */
		bytes = bytes + u8_wc_toutf8 (&(ib->buffer[bytes]), key->keysym.unicode);
		ib->buffer[bytes] = 0;
	}
	
	blanco.r = blanco.g = blanco.b = 255;
	
	if (ib->text_s != NULL) SDL_FreeSurface (ib->text_s);
	len = u8_strlen (ib->buffer);
	
	if (len > 0) {
		ib->text_s = TTF_RenderUTF8_Blended (ttf16_burbank_medium, ib->buffer, blanco);
	} else {
		ib->text_s = NULL;
	}
	
	/* Reiniciar el parpadeo */
	ib->cursor_vel = 0;
	//printf ("Ejecuto keydown, unicode: %hi. Cadena: \"%s\", len: %i\n", key->keysym.unicode, ib->buffer, strlen(ib->buffer));
	draw_inputbox_textfield (ib);
	return TRUE;
}

