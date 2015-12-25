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

int inputbox_mouse_down (InputBox *ib, int x, int y, int **button_map);
int inputbox_mouse_motion (InputBox *ib, int x, int y, int **button_map);
int inputbox_mouse_up (InputBox *ib, int x, int y, int **button_map);
void inputbox_draw (InputBox *ib, SDL_Surface *screen);
int inputbox_key_down (InputBox *ib, SDL_KeyboardEvent *);

InputBox *crear_inputbox (InputBoxFunc func, const char *ask, const char *text) {
	InputBox *ib;
	SDL_Color blanco;
	blanco.r = blanco.g = blanco.b = 255;
	
	ib = (InputBox *) malloc (sizeof (InputBox));
	
	ib->ventana.prev = NULL;
	ib->ventana.next = get_first_window ();
	/* Ancho mínimo 144, más múltiplos de 16 */
	/* Alto mínimo 52, más múltiplos de 8 */
	ib->ventana.h = 132;
	ib->ventana.x = ib->ventana.y = 0;
	
	ib->ventana.tipo = WINDOW_INPUT;
	ib->ventana.mostrar = TRUE;
	
	ib->ventana.mouse_down = (FindWindowMouseFunc) inputbox_mouse_down;
	ib->ventana.mouse_motion = (FindWindowMouseFunc) inputbox_mouse_motion;
	ib->ventana.mouse_up = (FindWindowMouseFunc) inputbox_mouse_up;
	ib->ventana.draw = (FindWindowDraw) inputbox_draw;
	ib->ventana.key_down = (FindWindowKeyFunc) inputbox_key_down;
	ib->ventana.key_up = NULL;
	
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
	
	ib->ventana.w = 144 + ((ib->text_ask->w / 16) + 1) * 16;
	ib->callback = func;
	
	if (get_first_window () == NULL) {
		set_first_window ((Ventana *) ib);
		set_last_window ((Ventana *) ib);
	} else {
		get_first_window ()->prev = (Ventana *) ib;
		set_first_window ((Ventana *) ib);
	}
	
	return ib;
}

void eliminar_inputbox (InputBox *ib) {
	/* Desligar completamente */
	Ventana *v = (Ventana *) ib;
	if (v->prev != NULL) {
		v->prev->next = v->next;
	} else {
		set_first_window (v->next);
	}
	
	if (v->next != NULL) {
		v->next->prev = v->prev;
	} else {
		set_last_window (v->prev);
	}
	
	/* Si hay algún indicativo a estos viejos botones, eliminarlo */
	if (cp_old_map == &(ib->close_frame)) {
		cp_old_map = NULL;
	}
	if (cp_last_button == &(ib->send_frame)) {
		cp_last_button = NULL;
	}
	
	if (ib->text_s != NULL) SDL_FreeSurface (ib->text_s);
	SDL_FreeSurface (ib->text_ask);
	
	stop_drag ((Ventana *) ib);
	free (ib);
}

int inputbox_mouse_down (InputBox *ib, int x, int y, int **button_map) {
	int p, q;
	if (ib->ventana.mostrar == FALSE) return FALSE;
	
	/* El click por el agarre se calcula */
	p = (ib->ventana.w - 104) / 2;
	q = ib->ventana.w - 68;
	if (x >= p && x < p + 104 && y < 22) {
		/* Click por el agarre */
		start_drag ((Ventana *) ib, x, y);
		return TRUE;
	} else if (x >= 22 + q && x < 50 + q && y >= ib->box_y && y < ib->box_y + 28) {
		*button_map = &(ib->send_frame);
		return TRUE;
	} else if (y >= 30 && y < 58 && x >= ib->ventana.w - 42 && x < ib->ventana.w - 14) {
		// El click cae en el botón de cierre de la ventana
		*button_map = &(ib->close_frame);
		return TRUE;
	} else if (y >= 16) {
		/* El evento cae dentro de la ventana */
		return TRUE;
	}
	
	return FALSE;
}

int inputbox_mouse_motion (InputBox *ib, int x, int y, int **button_map) {
	int p, q;
	if (ib->ventana.mostrar == FALSE) return FALSE;
	
	p = (ib->ventana.w - 104) / 2;
	q = ib->ventana.w - 68;
	
	if (y >= 30 && y < 58 && x >= ib->ventana.w - 42 && x < ib->ventana.w - 14) {
		*button_map = &(ib->close_frame);
		return TRUE;
	}
	
	if (x >= 22 + q && x < 50 + q && y >= ib->box_y && y < ib->box_y + 28) {
		*button_map = &(ib->send_frame);
		return TRUE;
	}
	
	if ((y >= 16) || (x >= p && x < p + 104)) {
		/* El evento cae dentro de la ventana */
		return TRUE;
	}
	
	return FALSE;
}

int inputbox_mouse_up (InputBox *ib, int x, int y, int **button_map) {
	int p, q;
	if (ib->ventana.mostrar == FALSE) return FALSE;
	
	/* El click por el agarre se calcula */
	p = (ib->ventana.w - 104) / 2;
	q = ib->ventana.w - 68;
	
	if (y >= 30 && y < 58 && x >= ib->ventana.w - 42 && x < ib->ventana.w - 14) {
		*button_map = &(ib->close_frame);
		if (cp_button_up (*button_map)) {
			eliminar_inputbox (ib);
		}
		return TRUE;
	}
	
	if (x >= 22 + q && x < 50 + q && y >= ib->box_y && y < ib->box_y + 28) {
		*button_map = &(ib->send_frame);
		if (cp_button_up (*button_map)) {
			if (ib->callback != NULL) {
				ib->callback (ib, ib->buffer);
			}
		}
		return TRUE;
	}
	
	if (y >= 16 || (x >= p && x < p + 104)) {
		/* El evento cae dentro de la ventana */
		return TRUE;
	}
	
	return FALSE;
}

void inputbox_draw (InputBox *ib, SDL_Surface *screen) {
	SDL_Rect rect, rect2, rect3;
	int p, q, g;
	Uint32 azul;
	
	p = (ib->ventana.w - 144) / 16;
	q = (ib->ventana.h - 52) / 8;
	/* Dibujar toda la ventana */
	
	/* Las 4 esquinas */
	rect.x = ib->ventana.x;
	rect.y = ib->ventana.y + 16;
	rect.w = 20;
	rect.h = 18;
	
	rect2.x = 0;
	rect2.y = 0;
	rect2.w = 20;
	rect2.h = 18;
	
	SDL_BlitSurface (images[IMG_WINDOW_PART], &rect2, screen, &rect);
	
	rect.x = ib->ventana.x + ib->ventana.w - 20;
	rect.y = ib->ventana.y + 16;
	rect.w = 20;
	rect.h = 18;
	
	rect2.x = 28;
	rect2.y = 0;
	
	SDL_BlitSurface (images[IMG_WINDOW_PART], &rect2, screen, &rect);
	
	rect.x = ib->ventana.x;
	rect.y = ib->ventana.y + ib->ventana.h - 18;
	rect.w = 20;
	rect.h = 18;
	
	rect2.x = 0;
	rect2.y = 26;
	
	SDL_BlitSurface (images[IMG_WINDOW_PART], &rect2, screen, &rect);
	
	rect.x = ib->ventana.x + ib->ventana.w - 20;
	rect.y = ib->ventana.y + ib->ventana.h - 18;
	rect.w = 20;
	rect.h = 18;
	
	rect2.x = 28;
	rect2.y = 26;
	
	SDL_BlitSurface (images[IMG_WINDOW_PART], &rect2, screen, &rect);
	
	/* Parte superior */
	rect2.w = 8;
	rect2.h = 18;
	rect2.x = 20;
	rect2.y = 0;
	
	for (g = 0; g < p; g++) {
		rect.x = ib->ventana.x + 20 + (g * 8);
		rect.y = ib->ventana.y + 16;
		rect.w = 8;
		rect.h = 18;
		
		SDL_BlitSurface (images[IMG_WINDOW_PART], &rect2, screen, &rect);
		
		rect.x = ib->ventana.x + 20 + (g * 8) + 104 + (p * 8);
		rect.y = ib->ventana.y + 16;
		rect.w = 8;
		rect.h = 18;
		
		SDL_BlitSurface (images[IMG_WINDOW_PART], &rect2, screen, &rect);
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
		rect.x = ib->ventana.x;
		rect.y = ib->ventana.y + 34 + (g * 8);
		rect.w = 20;
		rect.h = 8;
		
		SDL_BlitSurface (images[IMG_WINDOW_PART], &rect2, screen, &rect);
		
		rect.x = ib->ventana.x + ib->ventana.w - 20;
		rect.y = ib->ventana.y + 34 + (g * 8);
		rect.w = 20;
		rect.h = 8;
		
		SDL_BlitSurface (images[IMG_WINDOW_PART], &rect3, screen, &rect);
	}
	
	/* Lado inferior */
	rect2.x = 20;
	rect2.y = 26;
	rect2.w = 8;
	rect2.h = 18;
	for (g = 0; g < p + p + 13; g++) {
		rect.x = ib->ventana.x + 20 + (g * 8);
		rect.y = ib->ventana.y + ib->ventana.h - 18;
		rect.w = 8;
		rect.h = 18;
		
		SDL_BlitSurface (images[IMG_WINDOW_PART], &rect2, screen, &rect);
	}
	
	/* Dibujar el agarre */
	rect.x = ib->ventana.x + 20 + (p * 8);
	rect.y = ib->ventana.y;
	rect.w = images[IMG_WINDOW_TAB]->w;
	rect.h = images[IMG_WINDOW_TAB]->h;
	SDL_BlitSurface (images[IMG_WINDOW_TAB], NULL, screen, &rect);
	
	/* Rellenar todo el centro de color sólido */
	azul = SDL_MapRGB (screen->format, 0x02, 0x80, 0xcd);
	
	rect.x = ib->ventana.x + 20;
	rect.w = ib->ventana.w - 40;
	rect.y = ib->ventana.y + 34;
	rect.h = ib->ventana.h - 52;
	
	SDL_FillRect (screen, &rect, azul);
	
	/* Ahora dibujar el inputbox */
	p = (ib->ventana.w - 68) / 10;
	
	rect.x = ib->ventana.x + 22;
	rect.y = ib->ventana.y + ib->box_y;
	rect.w = 14;
	rect.h = 27;
	
	rect2.x = rect2.y = 0;
	rect2.w = 14;
	rect2.h = 27;
	
	SDL_BlitSurface (images[IMG_INPUT_BOX], &rect2, screen, &rect);
	
	rect2.x = 14;
	rect2.y = 0;
	rect2.w = 10;
	rect2.h = 27;
	
	for (g = 0; g < p; g++) {
		rect.x = ib->ventana.x + 36 + (g * 10);
		rect.y = ib->ventana.y + ib->box_y;
		rect.w = 10;
		rect.h = 27;
		
		SDL_BlitSurface (images[IMG_INPUT_BOX], &rect2, screen, &rect);
	}
	
	/* Botón de enviar */
	rect.x = ib->ventana.x + 22 + (p * 10);
	rect.y = ib->ventana.y + ib->box_y;
	rect.w = images[IMG_BUTTON_LIST_UP]->w;
	rect.h = images[IMG_BUTTON_LIST_UP]->h;
	
	SDL_BlitSurface (images[ib->send_frame], NULL, screen, &rect);
	
	rect.x = ib->ventana.x + 30 + (p * 10);
	rect.y = ib->ventana.y + ib->box_y + 7;
	rect.w = images[IMG_BUTTON_LIST_UP]->w;
	rect.h = images[IMG_BUTTON_LIST_UP]->h;
	
	SDL_BlitSurface (images[IMG_CHAT_MINI], NULL, screen, &rect);
	
	/* Dibujar la pregunta de texto */
	rect.x = ib->ventana.x + 26;
	rect.y = ib->ventana.y + 42;
	rect.w = ib->text_ask->w;
	rect.h = ib->text_ask->h;
	
	SDL_BlitSurface (ib->text_ask, NULL, screen, &rect);
	
	/* Dibujar la cadena de texto */
	if (ib->text_s != NULL) {
		rect.x = ib->ventana.x + 36;
		rect.y = ib->ventana.y + ib->box_y + 8;
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
		SDL_BlitSurface (ib->text_s, &rect2, screen, &rect);
	}
	
	/* Dibujar el botón de cierre */
	rect.x = ib->ventana.x + ib->ventana.w - 42;
	rect.y = ib->ventana.y + 30;
	rect.w = images[IMG_BUTTON_CLOSE_UP]->w;
	rect.h = images[IMG_BUTTON_CLOSE_UP]->h;
	
	SDL_BlitSurface (images[ib->close_frame], NULL, screen, &rect);
}

int inputbox_key_down (InputBox *ib, SDL_KeyboardEvent *key) {
	int len, g;
	int bytes, bytes2;
	SDL_Color blanco;
	
	if (key->keysym.unicode == 0) return FALSE;
	
	if (key->keysym.unicode != 8 && (key->keysym.unicode < 32 || key->keysym.unicode == 127)) return TRUE; /* Tecla de eliminar */
	
	if (key->keysym.unicode == 13) return TRUE;
	
	if (key->keysym.unicode == 8) {
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
	//printf ("Ejecuto keydown, unicode: %hi. Cadena: \"%s\", len: %i\n", key->keysym.unicode, ib->buffer, strlen(ib->buffer));
	return TRUE;
}

