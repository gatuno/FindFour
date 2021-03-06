/*
 * findfour.h
 * This file is part of Find Four
 *
 * Copyright (C) 2015 - Félix Arreola Rodríguez
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __FINDFOUR_H__
#define __FINDFOUR_H__

#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>

#define NICK_SIZE 16

#define RANDOM(x) ((int) (x ## .0 * rand () / (RAND_MAX + 1.0)))

#ifndef FALSE
#	define FALSE 0
#endif

#ifndef TRUE
#	define TRUE !FALSE
#endif

/* Enumerar las imágenes */
enum {
	IMG_WINDOW,
	
	IMG_BOARD,
	IMG_COINBLUE,
	IMG_COINRED,
	
	IMG_BIGCOINBLUE,
	IMG_BIGCOINRED,
	
	IMG_WINDOW_CHAT,
	
	IMG_SHADOW_UP,
	IMG_SHADOW_OVER,
	IMG_SHADOW_DOWN,
	
	IMG_BUTTON_CLOSE_UP,
	IMG_BUTTON_CLOSE_OVER,
	IMG_BUTTON_CLOSE_DOWN,
	
	IMG_BUTTON_ARROW_1_UP,
	IMG_BUTTON_ARROW_1_OVER,
	IMG_BUTTON_ARROW_1_DOWN,
	
	IMG_BUTTON_ARROW_2_UP,
	IMG_BUTTON_ARROW_2_OVER,
	IMG_BUTTON_ARROW_2_DOWN,
	
	IMG_BUTTON_LIST_UP,
	IMG_BUTTON_LIST_OVER,
	IMG_BUTTON_LIST_DOWN,
	
	IMG_LOADING,
	
	IMG_LIST_MINI,
	IMG_LIST_RECENT_MINI,
	IMG_LIST_BIG,
	
	IMG_WIN_1,
	IMG_WIN_2,
	IMG_WIN_3,
	IMG_WIN_4,
	
	IMG_WINDOW_PART,
	IMG_WINDOW_PART_SHADOW,
	IMG_WINDOW_TAB,
	IMG_INPUT_BOX,
	
	IMG_CHAT_MINI,
	
	IMG_WINDOW_ERROR,
	IMG_WINDOW_ERROR_SHADOW,
	IMG_BUTTON_ERROR_UP,
	IMG_BUTTON_ERROR_OVER,
	IMG_BUTTON_ERROR_DOWN,
	
	IMG_BUTTON_NORMAL_UP,
	IMG_BUTTON_NORMAL_OVER,
	IMG_BUTTON_NORMAL_DOWN,
	
	IMG_FUZZ,
	
	NUM_IMAGES
};

enum {
	SND_DROP,
	SND_WIN,
	
	NUM_SOUNDS
};

extern SDL_Surface * images [NUM_IMAGES];
extern char nick_global[NICK_SIZE];
extern SDL_Surface *nick_image, *nick_image_blue;
extern SDL_Surface * text_waiting;

extern int use_sound;
extern Mix_Chunk * sounds[NUM_SOUNDS];

extern TTF_Font *ttf16_burbank_medium, *ttf14_facefront, *ttf16_comiccrazy, *ttf20_comiccrazy;

#endif /* __FINDFOUR_H__ */

