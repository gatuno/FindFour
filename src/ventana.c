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

#include "background.h"

#ifndef SDL_FALSE
#define SDL_FALSE 0
#endif

#ifndef SDL_TRUE
#define SDL_TRUE -1
#endif

struct _Ventana {
	SDL_Surface *surface;
	
	/* Coordenadas de la ventana */
	SDL_Rect coords;
	
	/* Rectangulo de actualización */
	SDL_Rect draw_update;
	
	int mostrar;
	
	/* Para el nuevo motor de botones */
	int button_start;
	int *buttons_frame;
	
	/* Data "extra" que cargan las ventanas */
	void *data;
	
	/* Si tiene el timer activo o no */
	int timer_active;
	
	/* Manejadores de la ventana */
	FindWindowMouseFunc mouse_down;
	FindWindowMouseFunc mouse_motion;
	FindWindowMouseFunc mouse_up;
	FindWindowKeyFunc key_down;
	FindWindowKeyFunc key_up;
	FindWindowTimerCallback timer_callback;
	FindWindowFocusEvent focus_callback;
	
	/* Manejadores de los botones */
	FindWindowButtonFrameChange button_frame_func;
	FindWindowButtonEvent button_event_func;
	
	/* Para la lista ligada */
	Ventana *prev, *next;
};

/* Para almacenar cuál botón está presionado y en cuál ventana */
typedef struct {
	int button;
	Ventana *ventana;
} CPButton;

/* Controlan la posición inicial de las ventanas */
static int start_x = 0, start_y = 0;

/* Nuestra lista ligada de ventanas */
static Ventana *primero = NULL, *ultimo = NULL;

/* Nuestra ventana actualmente arrastrada */
static Ventana *drag = NULL;
/* Para controlar el offset de arrastre */
static int drag_x, drag_y;

static int button_counter = 1;

static CPButton old_map = {0, NULL}, last_map = {0, NULL}, for_process;

#define MAX_RECTS 64

static SDL_Rect update_rects[MAX_RECTS];
static int num_rects = 0;

/* Funciones */
int SDL_RectEmpty(const SDL_Rect *r) {
	return ((!r) || (r->w <= 0) || (r->h <= 0)) ? SDL_TRUE : SDL_FALSE;
}

int SDL_HasIntersection(const SDL_Rect * A, const SDL_Rect * B) {
	int Amin, Amax, Bmin, Bmax;

	if (!A) {
		return SDL_FALSE;
	}

	if (!B) {
		return SDL_FALSE;
	}

	/* Special cases for empty rects */
	if (SDL_RectEmpty(A) || SDL_RectEmpty(B)) {
		return SDL_FALSE;
	}

	/* Horizontal intersection */
	Amin = A->x;
	Amax = Amin + A->w;
	Bmin = B->x;
	Bmax = Bmin + B->w;
	if (Bmin > Amin)
		Amin = Bmin;
	if (Bmax < Amax)
		Amax = Bmax;
	if (Amax <= Amin)
		return SDL_FALSE;

	/* Vertical intersection */
	Amin = A->y;
	Amax = Amin + A->h;
	Bmin = B->y;
	Bmax = Bmin + B->h;
	if (Bmin > Amin)
		Amin = Bmin;
	if (Bmax < Amax)
		Amax = Bmax;
	if (Amax <= Amin)
		return SDL_FALSE;

	return SDL_TRUE;
}

int SDL_IntersectRect(const SDL_Rect * A, const SDL_Rect * B, SDL_Rect * result) {
	int Amin, Amax, Bmin, Bmax;

	if (!A) {
		return SDL_FALSE;
	}

	if (!B) {
		return SDL_FALSE;
	}

	if (!result) {
		return SDL_FALSE;
	}

	/* Special cases for empty rects */
	if (SDL_RectEmpty(A) || SDL_RectEmpty(B)) {
		result->w = 0;
		result->h = 0;
		return SDL_FALSE;
	}

	/* Horizontal intersection */
	Amin = A->x;
	Amax = Amin + A->w;
	Bmin = B->x;
	Bmax = Bmin + B->w;
	if (Bmin > Amin)
		Amin = Bmin;
	result->x = Amin;
	if (Bmax < Amax)
		Amax = Bmax;
	if (Amax - Amin < 0) {
		result->w = 0;
	} else {
		result->w = Amax - Amin;
	}

	/* Vertical intersection */
	Amin = A->y;
	Amax = Amin + A->h;
	Bmin = B->y;
	Bmax = Bmin + B->h;
	if (Bmin > Amin)
		Amin = Bmin;
	result->y = Amin;
	if (Bmax < Amax)
		Amax = Bmax;
	if (Amax - Amin < 0) {
		result->h = 0;
	} else {
		result->h = Amax - Amin;
	}

	return !SDL_RectEmpty(result);
}

void SDL_UnionRect(const SDL_Rect * A, const SDL_Rect * B, SDL_Rect * result) {
	int Amin, Amax, Bmin, Bmax;

	if (!A) {
		return;
	}

	if (!B) {
		return;
	}

	if (!result) {
		return;
	}

	/* Special cases for empty Rects */
	if (SDL_RectEmpty(A)) {
		if (SDL_RectEmpty(B)) {
			/* A and B empty */
			return;
		} else {
			/* A empty, B not empty */
			*result = *B;
			return;
		}
	} else {
		if (SDL_RectEmpty(B)) {
			/* A not empty, B empty */
			*result = *A;
			return;
		}
	}

	/* Horizontal union */
	Amin = A->x;
	Amax = Amin + A->w;
	Bmin = B->x;
	Bmax = Bmin + B->w;
	if (Bmin < Amin)
		Amin = Bmin;
	result->x = Amin;
	if (Bmax > Amax)
		Amax = Bmax;
	result->w = Amax - Amin;

	/* Vertical union */
	Amin = A->y;
	Amax = Amin + A->h;
	Bmin = B->y;
	Bmax = Bmin + B->h;
	if (Bmin < Amin)
		Amin = Bmin;
	result->y = Amin;
	if (Bmax > Amax)
		Amax = Bmax;
	result->h = Amax - Amin;
}

void window_manager_background_update (SDL_Rect *rect) {
	update_rects[num_rects] = *rect;
	
	num_rects++;
}

Ventana *window_create (int w, int h, int top_window) {
	Ventana *nueva;
	
	nueva = (Ventana *) malloc (sizeof (Ventana));
	
	/* Reservar la superficie para dibujar */
	nueva->surface = SDL_AllocSurface (SDL_SWSURFACE, w, h, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	//SDL_SetAlpha (nueva->surface, 0, 0);
	
	nueva->coords.w = w;
	nueva->coords.h = h;
	
	if (start_y + h >= 480) {
		start_y = 0;
		start_x += 20;
	}
	
	if (start_x + w >= 760) {
		start_x = 0;
	}
	
	nueva->coords.x = start_x;
	nueva->coords.y = start_y;
	
	nueva->draw_update = nueva->coords;
	
	start_x += 20;
	start_y += 20;
	
	nueva->mostrar = TRUE;
	
	nueva->timer_active = FALSE;
	
	if (top_window || primero == NULL) {
		/* Si pidieron ser ventana al frente, mostrarla */
		nueva->prev = NULL;
		nueva->next = primero;
		if (primero == NULL) {
			primero = nueva;
			ultimo = nueva;
		} else {
			/* Correr aquí el evento de perder foco de la ventana */
			if (primero->focus_callback != NULL) {
				primero->focus_callback (primero, FALSE);
			}
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
	nueva->timer_callback = NULL;
	nueva->focus_callback = NULL;
	
	nueva->data = NULL;
	
	/* Información de los botones */
	nueva->button_start = 0;
	nueva->buttons_frame = NULL;
	
	nueva->button_frame_func = NULL;
	nueva->button_event_func = NULL;
	
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

void window_update (Ventana *v, SDL_Rect *rect) {
	SDL_UnionRect (&(v->draw_update), rect, &(v->draw_update));
}

void window_flip (Ventana *v) {
	v->draw_update = v->coords;
	v->draw_update.x -= v->coords.x;
	v->draw_update.y -= v->coords.y;
}

void window_hide (Ventana *v) {
	v->mostrar = FALSE;
	window_manager_background_update (&v->coords);
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

void window_register_timer_events (Ventana *v, FindWindowTimerCallback cb) {
	v->timer_callback = cb;
}

void window_register_buttons (Ventana *v, int count, FindWindowButtonFrameChange frame, FindWindowButtonEvent event) {
	if (v->button_start != 0) {
		printf ("Advertencia, la ventana ya tiene registrados botones, ignorando\n");
		return;
	}
	
	v->button_start = button_counter;
	
	button_counter += count;
	
	v->button_frame_func = frame;
	v->button_event_func = event;
	
	v->buttons_frame = (int *) malloc (sizeof (int) * count);
	memset (v->buttons_frame, 0, sizeof (int) * count);
}

void window_register_focus_events (Ventana *v, FindWindowFocusEvent event) {
	v->focus_callback = event;
}

void window_button_mouse_down (Ventana *v, int button) {
	/* Guardar el botón para el procesamiento posterior */
	if (v == NULL) return;
	
	for_process.button = button;
	for_process.ventana = v;
}

void window_button_down_process (CPButton *p) {
	last_map.button = p->button;
	last_map.ventana = p->ventana;
	
	if (last_map.ventana != NULL) {
		/* Disparar el evento de frame ++ */
		last_map.ventana->buttons_frame[last_map.button]++;
		/* Dispara redibujado del botón */
		last_map.ventana->button_frame_func (last_map.ventana, last_map.button, last_map.ventana->buttons_frame[last_map.button]);
	}
}

void window_button_mouse_motion (Ventana *v, int button) {
	if (v == NULL) return;
	
	for_process.button = button;
	for_process.ventana = v;
}

void window_button_motion_process (CPButton *p) {
	/* Motor de botones */
	if (old_map.ventana == NULL && p->ventana != NULL) {
		if (last_map.ventana == NULL || (last_map.ventana == p->ventana && last_map.button == p->button)) {
			p->ventana->buttons_frame[p->button]++;
			p->ventana->button_frame_func (p->ventana, p->button, p->ventana->buttons_frame[p->button]);
		}
	} else if (old_map.ventana != NULL && p->ventana == NULL) {
		if (last_map.ventana == NULL) {
			old_map.ventana->buttons_frame[old_map.button]--;
			
			old_map.ventana->button_frame_func (old_map.ventana, old_map.button, old_map.ventana->buttons_frame[old_map.button]);
		} else if (last_map.ventana == old_map.ventana && last_map.button == old_map.button) {
			last_map.ventana->buttons_frame[last_map.button]--;
			
			last_map.ventana->button_frame_func (last_map.ventana, last_map.button, last_map.ventana->buttons_frame[last_map.button]);
		}
	} else if (old_map.ventana != p->ventana || old_map.button != p->button) {
		if (last_map.ventana == NULL) {
			p->ventana->buttons_frame[p->button]++;
			p->ventana->button_frame_func (p->ventana, p->button, p->ventana->buttons_frame[p->button]);
			if (old_map.ventana != NULL) {
				old_map.ventana->buttons_frame[old_map.button]--;
				old_map.ventana->button_frame_func (old_map.ventana, old_map.button, old_map.ventana->buttons_frame[old_map.button]);
			}
		} else if (last_map.ventana == old_map.ventana && last_map.button == old_map.button) {
			old_map.ventana->buttons_frame[old_map.button]--;
			old_map.ventana->button_frame_func (old_map.ventana, old_map.button, old_map.ventana->buttons_frame[old_map.button]);
		} else if (last_map.ventana == p->ventana && last_map.button == p->button) {
			p->ventana->buttons_frame[p->button]++;
			p->ventana->button_frame_func (p->ventana, p->button, p->ventana->buttons_frame[p->button]);
		}
	}
	old_map = *p;
}

void window_button_mouse_up (Ventana *v, int button) {
	if (v == NULL) return;
	
	for_process.button = button;
	for_process.ventana = v;
}

void window_button_up_process (CPButton *p) {
	if (last_map.ventana != NULL) {
		
		last_map.ventana->buttons_frame[last_map.button]--;
		last_map.ventana->button_frame_func (last_map.ventana, last_map.button, last_map.ventana->buttons_frame[last_map.button]);
		if (last_map.ventana == p->ventana && last_map.button == p->button) {
			/* Disparar el evento del botón */
			p->ventana->button_event_func (p->ventana, p->button);
		} else if (p->ventana != NULL) {
			p->ventana->buttons_frame[p->button]++;
			p->ventana->button_frame_func (p->ventana, p->button, p->ventana->buttons_frame[p->button]);
		}
		
		last_map.ventana = NULL;
		last_map.button = 0;
	}
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
	/* Si hay algún botón que apunte a esta ventana, eliminar la referencia */
	if (last_map.ventana == v) {
		last_map.ventana = NULL;
		last_map.button = 0;
	}
	
	if (old_map.ventana == v) {
		old_map.ventana = NULL;
		old_map.button = 0;
	}
	
	/* Si esta ventana era la que se estaba arrastrando, detener el arrastre */
	if (drag == v) drag = NULL;
	
	if (v->prev != NULL) {
		v->prev->next = v->next;
	} else {
		primero = v->next;
		
		if (primero != NULL && primero->focus_callback != NULL) {
			primero->focus_callback (primero, TRUE);
		}
	}
	
	if (v->next != NULL) {
		v->next->prev = v->prev;
	} else {
		ultimo = v->prev;
	}
	
	window_manager_background_update (&v->coords);
	
	free (v);
}

void window_raise (Ventana *v) {
	if (v != primero) {
		/* Disparar el evento de "perder foco" en la ventana primero */
		if (primero->focus_callback != NULL) {
			primero->focus_callback (primero, FALSE);
		}
		
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
		
		/* Marcar la ventana como para dibujar todo */
		primero->draw_update.x = primero->draw_update.y = 0;
		primero->draw_update.w = primero->coords.w;
		primero->draw_update.h = primero->coords.h;
		
		if (primero->focus_callback != NULL) {
			primero->focus_callback (primero, TRUE);
		}
	}
}

void window_enable_timer (Ventana *v) {
	v->timer_active = TRUE;
}

void window_disable_timer (Ventana *v) {
	v->timer_active = FALSE;
}

void window_manager_event (SDL_Event event) {
	int manejado;
	Ventana *ventana, *next;
	int x, y;
	
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
			
			/* Para los botones, antes del mouse down asumir que nadie está presionado */
			for_process.button = 0;
			for_process.ventana = NULL;
			
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
				if (x >= ventana->coords.x && x < ventana->coords.x + ventana->coords.w && y >= ventana->coords.y && y < ventana->coords.y + ventana->coords.h) {
					/* Pasarlo al manejador de la ventana */
					if (ventana->mouse_down != NULL) {
						manejado = ventana->mouse_down (ventana, x - ventana->coords.x, y - ventana->coords.y);
					}
					
					if (manejado && primero != ventana) {
						/* Si el evento cayó dentro de esta ventana, levantar la ventana */
						window_raise (ventana);
					}
				}
			}
			
			/* Procesar el mouse down */
			window_button_down_process (&for_process);
			break;
		case SDL_MOUSEMOTION:
			/* Para los botones, antes del mouse motion asumir que nadie está presionado */
			for_process.button = 0;
			for_process.ventana = NULL;
			
			if (drag != NULL) {
				/* Programar el redibujado del fondo */
				window_manager_background_update (&drag->coords);
				
				/* Mover la ventana a las coordenadas del mouse - los offsets */
				drag->coords.x = event.motion.x - drag_x;
				drag->coords.y = event.motion.y - drag_y;
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
					if (x >= ventana->coords.x && x < ventana->coords.x + ventana->coords.w && y >= ventana->coords.y && y < ventana->coords.y + ventana->coords.h) {
						if (ventana->mouse_motion != NULL) {
							manejado = ventana->mouse_motion (ventana, x - ventana->coords.x, y - ventana->coords.y);
						}
					}
				}
				
				/* Recorrer las ventanas restantes y mandarles un evento mouse motion nulo */
				while (ventana != NULL) {
					if (ventana->mouse_motion != NULL) {
						ventana->mouse_motion (ventana, -1, -1);
					}
					
					ventana = ventana->next;
				}
				if (!manejado) { /* A ver si el fondo quiere manejar este evento */
					background_mouse_motion (x, y);
				}
			}
			
			window_button_motion_process (&for_process);
			
			break;
		case SDL_MOUSEBUTTONUP:
			if (event.button.button != SDL_BUTTON_LEFT) break;
			drag = NULL;
			manejado = FALSE;
			
			/* Para los botones, antes del mouse motion asumir que nadie está presionado */
			for_process.button = 0;
			for_process.ventana = NULL;
			
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
				if (x >= ventana->coords.x && x < ventana->coords.x + ventana->coords.w && y >= ventana->coords.y && y < ventana->coords.y + ventana->coords.h) {
					if (ventana->mouse_up != NULL) {
						manejado = ventana->mouse_up (ventana, x - ventana->coords.x, y - ventana->coords.y);
					}
				}
				ventana = next;
			}
			
			window_button_up_process (&for_process);
			
			break;
	}
}

void window_manager_timer (void) {
	Ventana *ventana;
	int res;
	
	for (ventana = ultimo; ventana != NULL; ventana = ventana->prev) {
		if (ventana->timer_active && ventana->timer_callback != NULL) {
			/* Llamar al timer de la ventana */
			res = ventana->timer_callback (ventana);
			
			if (res == FALSE) {
				ventana->timer_active = FALSE;
			}
		}
	}
}

void window_manager_draw (SDL_Surface *screen) {
	Ventana *ventana;
	SDL_Rect rect, rect2;
	int g;
	int whole_flip = 0;
	
	update_background (screen);
	
	/* Primero, recolectar todos los rectangulos de actualización */
	for (ventana = ultimo; ventana != NULL && whole_flip == 0; ventana = ventana->prev) {
		if (!ventana->mostrar) {
			ventana->draw_update.w = ventana->draw_update.h = 0;
			continue;
		}
		
		if (SDL_RectEmpty (&ventana->draw_update)) continue;
		
		if (num_rects < MAX_RECTS) {
			rect = ventana->draw_update;
			
			rect.x += ventana->coords.x;
			rect.y += ventana->coords.y;
			
			update_rects[num_rects] = rect;
			
			num_rects++;
			
			if (num_rects == MAX_RECTS) {
				/* Se activa el whole flip */
				whole_flip = 1;
			}
		}
		
		ventana->draw_update.w = ventana->draw_update.h = 0;
	}
	
	if (whole_flip) {
		/* Dibujar todo directamente */
		draw_background (screen, NULL);
		
		/* Recorrer las ventanas y dibujarlas todas */
		for (ventana = ultimo; ventana != NULL; ventana = ventana->prev) {
			if (!ventana->mostrar) continue;
			
			SDL_BlitSurface (ventana->surface, NULL, screen, &ventana->coords);
		}
		
		SDL_Flip (screen);
	} else {
		for (g = 0; g < num_rects; g++) {
			/* Redibujar la porción de fondo que es necesaria */
			draw_background (screen, &update_rects[g]);
			
			for (ventana = ultimo; ventana != NULL; ventana = ventana->prev) {
				if (!ventana->mostrar) continue;
				
				if (SDL_IntersectRect (&update_rects[g], &ventana->coords, &rect)) {
					/* Redibujar la porción de la ventana que requiere ser redibujada */
					
					rect2 = rect;
					/* Origen */
					rect2.x -= ventana->coords.x;
					rect2.y -= ventana->coords.y;
					
					SDL_BlitSurface (ventana->surface, &rect2, screen, &rect);
				}
			}
		}
		
		SDL_UpdateRects (screen, num_rects, update_rects);
	}
	
	num_rects = 0;
}
