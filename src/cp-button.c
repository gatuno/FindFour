/*
 * cp-button.c
 * This file is part of Thin Ice
 *
 * Copyright (C) 2013 - Felix Arreola Rodriguez
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <string.h>

int *cp_old_map, *cp_last_button;

void cp_button_start (void) {
	cp_old_map = cp_last_button = NULL;
}

void cp_button_motion (int *map) {
	/* Motor de botones */
	if (cp_old_map == NULL && map != NULL) {
		if (cp_last_button == NULL || cp_last_button == map) {
			(*map)++;
		}
	} else if (cp_old_map != NULL && map == NULL) {
		if (cp_last_button == NULL) {
			(*cp_old_map)--;
		} else if (cp_last_button == cp_old_map) {
			(*cp_last_button)--;
		}
	} else if (cp_old_map != map) {
		if (cp_last_button == NULL) {
			(*map)++;
			if (cp_old_map != NULL) {
				(*cp_old_map)--;
			}
		} else if (cp_last_button == cp_old_map) {
			(*cp_old_map)--;
		} else if (cp_last_button == map) {
			(*map)++;
		}
	}
	cp_old_map = map;
}

void cp_button_down (int *map) {
	cp_last_button = map;
	
	if (cp_last_button != NULL) {
		(*cp_last_button)++;
	}
}

int cp_button_up (int *map) {
	if (cp_last_button != NULL) {
		(*cp_last_button)--;
		if (map == cp_last_button) {
			/* Switch del boton */
			cp_last_button = NULL;
			return 1; /* Este botón sí fué presionado */
		} else if (map != NULL) {
			(*map)++;
		}
		
		cp_last_button = NULL;
	}
	
	return 0;
}
