/*
* Copyright (C) 1996-1997 Id Software, Inc.
 * Copyright (C) 2025 Henrique Barateli <henriquejb194@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */


#ifndef __ES_PALETTE__
#define __ES_PALETTE__

#include "quakedef.h"
#include <SDL_surface.h>

typedef enum {
    PAL_COLOR_BLACK,
    PAL_COLOR_BLUE,
    PAL_COLOR_GREEN,
    PAL_COLOR_CYAN,
    PAL_COLOR_RED,
    PAL_COLOR_MAGENTA,
    PAL_COLOR_BROWN,
    PAL_COLOR_LIGHTGRAY,
    PAL_COLOR_DARKGRAY,
    PAL_COLOR_LIGHTBLUE,
    PAL_COLOR_LIGHTGREEN,
    PAL_COLOR_LIGHTCYAN,
    PAL_COLOR_LIGHTRED,
    PAL_COLOR_LIGHTMAGENTA,
    PAL_COLOR_YELLOW,
    PAL_COLOR_WHITE,
    PAL_COLOR_NUM,
} palette_color_t;

void ES_SetPalette(const SDL_Surface* buffer);

#endif
