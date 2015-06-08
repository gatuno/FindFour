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

#include "findfour.h"

/* Estructuras */
typedef struct _Chat {
	Ventana ventana;
	
	/* El botón de cierre */
	int close_frame; /* ¿El refresh es necesario? */
	
	/* El bóton de flecha hacia arriba y hacia abajo */
	int up_frame, down_frame;
	
	/* El botón de buddy locales */
	int broadcast_list_frame;
	
	/* Los 8 buddys */
	int buddys[8];
} Chat;

Chat *inicializar_chat (void);

#endif /* __CHAT_H__ */

