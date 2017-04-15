/*
 * ventana.c
 * This file is part of Find Four
 *
 * Copyright (C) 2017 - Félix Arreola Rodríguez
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
 * along with Find Four; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */

#include <SDL.h>

#include "findfour.h"

#include "ventana.h"

struct _Ventana {
	int tipo;
	
	SDL_Surface *surface;
	int needs_redraw;
	
	/* Coordenadas de la ventana */
	int x, y;
	int w, h;
	
	int mostrar;
	
	void *data;
	
	/* Manejadores de la ventana */
	FindWindowMouseFunc mouse_down;
	FindWindowMouseFunc mouse_motion;
	FindWindowMouseFunc mouse_up;
	FindWindowKeyFunc key_down;
	FindWindowKeyFunc key_up;
	
	/* Para la lista ligada */
	Ventana *prev, *next;
};

/* Controlan la posición inicial de las ventanas */
static int start_x = 0, start_y = 0;

/* Nuestra lista ligada de ventanas */
static Ventana *primero = NULL, *ultimo = NULL;

/* Nuestra ventana actualmente arrastrada */
static Ventana *drag = NULL;
/* Para controlar el offset de arrastre */
static int drag_x, drag_y;

Ventana *window_create (int w, int h, int top_window) {
	Ventana *nueva;
	
	nueva = (Ventana *) malloc (sizeof (Ventana));
	
	/* Reservar la superficie para dibujar */
	nueva->surface = SDL_AllocSurface (SDL_SWSURFACE, w, h, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	//SDL_SetAlpha (nueva->surface, 0, 0);
	
	nueva->needs_redraw = TRUE;
	
	nueva->w = w;
	nueva->h = h;
	
	if (start_y + h >= 480) {
		start_y = 0;
		start_x += 20;
	}
	
	if (start_x + w >= 760) {
		start_x = 0;
	}
	
	nueva->x = start_x;
	nueva->y = start_y;
	
	start_x += 20;
	start_y += 20;
	
	nueva->mostrar = TRUE;
	
	if (top_window || primero == NULL) {
		/* Si pidieron ser ventana al frente, mostrarla */
		nueva->prev = NULL;
		nueva->next = primero;
		if (primero == NULL) {
			primero = nueva;
		} else {
			primero->prev = nueva;
		}
		
		primero = nueva;
	} else {
		/* En caso contrario, mandarla hacia abajo de la primera ventana */
		nueva->prev = primero;
		nueva->next = primero->next;
		
		primero->next = nueva;
		
		/* Si tiene next, cambiar su prev */
		if (nueva->next == NULL) {
			ultimo = nueva;
		} else {
			nueva->next->prev = nueva;
		}
	}
	
	/* Los manejadores de eventos */
	nueva->mouse_down = nueva->mouse_motion = nueva->mouse_up = NULL;
	
	nueva->key_down = nueva->key_up = NULL;
	nueva->data = NULL;
	
	return nueva;
}

void window_set_data (Ventana *v, void *data) {
	v->data = data;
}

void * window_get_data (Ventana *v) {
	return v->data;
}

SDL_Surface * window_get_surface (Ventana *v) {
	return v->surface;
}

void window_want_redraw (Ventana *v) {
	v->needs_redraw = TRUE;
}

void window_hide (Ventana *v) {
	v->mostrar = FALSE;
}

void window_show (Ventana *v) {
	v->mostrar = TRUE;
}

void window_register_mouse_events (Ventana *v, FindWindowMouseFunc down, FindWindowMouseFunc motion, FindWindowMouseFunc up) {
	v->mouse_down = down;
	v->mouse_motion = motion;
	v->mouse_up = up;
}

void window_register_keyboard_events (Ventana *v, FindWindowKeyFunc down, FindWindowKeyFunc up) {
	v->key_down = down;
	v->key_up = up;
}

void window_start_drag (Ventana *v, int offset_x, int offset_y) {
	/* Cuando una ventana determina que se va a mover, guardamos su offset para un arrastre perfecto */
	drag_x = offset_x;
	drag_y = offset_y;
	
	drag = v;
}

void window_cancel_draging (void) {
	drag = NULL;
}

void window_destroy (Ventana *v) {
	/* Si esta ventana era la que se estaba arrastrando, detener el arrastre */
	if (drag == v) drag = NULL;
	
	if (v->prev != NULL) {
		v->prev->next = v->next;
	} else {
		primero = v->next;
	}
	
	if (v->next != NULL) {
		v->next->prev = v->prev;
	} else {
		ultimo = v->prev;
	}
	
	free (v);
}

void window_raise (Ventana *v) {
	if (v != primero) {
		/* Desligar completamente */
		if (v->prev != NULL) {
			v->prev->next = v->next;
		} else {
			primero = v->next;
		}
	
		if (v->next != NULL) {
			v->next->prev = v->prev;
		} else {
			ultimo = v->prev;
		}
	
		/* Levantar la ventana a primer plano */
		v->next = primero;
		primero->prev = v;
		v->prev = NULL;
		primero = v;
	}
}

void window_manager_event (SDL_Event event) {
	int manejado;
	Ventana *ventana, *next;
	int x, y;
	int *map;
	
	switch (event.type) {
		case SDL_KEYDOWN:
			/* Encontrar la primer ventana no oculta para enviarle el evento */
			manejado = FALSE;
			ventana = primero;
			
			while (ventana != NULL && ventana->mostrar != TRUE) ventana = ventana->next;
			
			//if (list_msg != NULL) ventana = NULL; /* No hay eventos de teclado cuando hay un mensaje en pantalla */
			
			if (ventana != NULL) {
				/* Revisar si le interesa nuestro evento */
				if (ventana->key_down != NULL) {
					manejado = ventana->key_down (ventana, &event.key);
				}
			}
			
			/* Si el evento aún no ha sido manejado por alguna ventana, es de nuestro interés */
			if (!manejado) {
				findfour_default_keyboard_handler (&event.key);
			}
			break;
		case SDL_MOUSEBUTTONDOWN:
			/* Primero, analizar si el evento cae dentro de alguna ventana */
			
			if (event.button.button != SDL_BUTTON_LEFT) break;
			map = NULL;
			manejado = FALSE;
			/*if (list_msg != NULL) {
				message_mouse_down (event.button.x, event.button.y, &map);
				manejado = TRUE;
			}*/
			x = event.button.x;
			y = event.button.y;
			
			for (ventana = primero; ventana != NULL && manejado == FALSE; ventana = ventana->next) {
				/* No hay eventos para ventanas que se están cerrando */
				if (!ventana->mostrar) continue;
				
				/* Si el evento hace match por las coordenadas, preguntarle a la ventana si lo quiere manejar */
				if (x >= ventana->x && x < ventana->x + ventana->w && y >= ventana->y && y < ventana->y + ventana->h) {
					/* Pasarlo al manejador de la ventana */
					if (ventana->mouse_down != NULL) {
						manejado = ventana->mouse_down (ventana, x - ventana->x, y - ventana->y, &map);
					}
					
					if (manejado && primero != ventana) {
						/* Si el evento cayó dentro de esta ventana, levantar la ventana */
						window_raise (ventana);
					}
				}
			}
			
			cp_button_down (map);
			break;
		case SDL_MOUSEMOTION:
			map = NULL; /* Para los botones de cierre */
			if (drag != NULL) {
				/* Mover la ventana a las coordenadas del mouse - los offsets */
				drag->x = event.motion.x - drag_x;
				drag->y = event.motion.y - drag_y;
			} else {
				manejado = FALSE;
				
				/*if (list_msg != NULL) {
					message_mouse_motion (event.motion.x, event.motion.y, &map);
					manejado = TRUE;
				}*/
				x = event.motion.x;
				y = event.motion.y;
				for (ventana = primero; ventana != NULL && manejado == FALSE; ventana = ventana->next) {
					/* Si el evento hace match por las coordenadas, preguntarle a la ventana si lo quiere manejar */
					if (x >= ventana->x && x < ventana->x + ventana->w && y >= ventana->y && y < ventana->y + ventana->h) {
						if (ventana->mouse_motion != NULL) {
							manejado = ventana->mouse_motion (ventana, x - ventana->x, y - ventana->y, &map);
						}
					}
				}
				
				/* Recorrer las ventanas restantes y mandarles un evento mouse motion nulo */
				while (ventana != NULL) {
					if (ventana->mouse_motion != NULL) {
						int *t;
						ventana->mouse_motion (ventana, -1, -1, &t);
					}
					
					ventana = ventana->next;
				}
				if (!manejado) { /* A ver si el fondo quiere manejar este evento */
					background_mouse_motion (x, y);
				}
			}
			
			cp_button_motion (map);
			break;
		case SDL_MOUSEBUTTONUP:
			if (event.button.button != SDL_BUTTON_LEFT) break;
			drag = NULL;
			manejado = FALSE;
			map = NULL;
			x = event.button.x;
			y = event.button.y;
			
			/*if (list_msg != NULL) {
				message_mouse_up (event.button.x, event.button.y, &map);
				manejado = TRUE;
			}*/
			
			ventana = primero;
			while (ventana != NULL && manejado == FALSE) {
				next = ventana->next;
				/* Si el evento hace match por las coordenadas, preguntarle a la ventana si lo quiere manejar */
				if (x >= ventana->x && x < ventana->x + ventana->w && y >= ventana->y && y < ventana->y + ventana->h) {
					if (ventana->mouse_up != NULL) {
						manejado = ventana->mouse_up (ventana, x - ventana->x, y - ventana->y, &map);
					}
				}
				ventana = next;
			}
			
			if (map == NULL) {
				cp_button_up (NULL);
			}
			break;
	}
}

void window_manager_draw (SDL_Surface *screen) {
	Ventana *ventana;
	SDL_Rect rect;
	
	draw_background (screen);
	
	for (ventana = ultimo; ventana != NULL; ventana = ventana->prev) {
		if (!ventana->mostrar) continue;
		
		/* Ahora nosotros dibujamos las superficies */
		rect.x = ventana->x;
		rect.y = ventana->y;
		rect.w = ventana->w;
		rect.h = ventana->h;
		
		SDL_BlitSurface (ventana->surface, NULL, screen, &rect);
	}
}
