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


#include "es_palette.h"

#define COLOR(red, green, blue)                                                \
    {.r = red, .g = green, .b = blue, .a = SDL_ALPHA_OPAQUE}

static const SDL_Color ega_colors[PAL_COLOR_NUM] = {
    [PAL_COLOR_BLACK] = COLOR(0x00, 0x00, 0x00),
    [PAL_COLOR_BLUE] = COLOR(0x00, 0x00, 0xa8),
    [PAL_COLOR_GREEN] = COLOR(0x00, 0xa8, 0x00),
    [PAL_COLOR_CYAN] = COLOR(0x00, 0xa8, 0xa8),
    [PAL_COLOR_RED] = COLOR(0xa8, 0x00, 0x00),
    [PAL_COLOR_MAGENTA] = COLOR(0xa8, 0x00, 0xa8),
    [PAL_COLOR_BROWN] = COLOR(0xa8, 0x54, 0x00),
    [PAL_COLOR_LIGHTGRAY] = COLOR(0xa8, 0xa8, 0xa8),
    [PAL_COLOR_DARKGRAY] = COLOR(0x54, 0x54, 0x54),
    [PAL_COLOR_LIGHTBLUE] = COLOR(0x54, 0x54, 0xfe),
    [PAL_COLOR_LIGHTGREEN] = COLOR(0x54, 0xfe, 0x54),
    [PAL_COLOR_LIGHTCYAN] = COLOR(0x54, 0xfe, 0xfe),
    [PAL_COLOR_LIGHTRED] = COLOR(0xfe, 0x54, 0x54),
    [PAL_COLOR_LIGHTMAGENTA] = COLOR(0xfe, 0x54, 0xfe),
    [PAL_COLOR_YELLOW] = COLOR(0xfe, 0xfe, 0x54),
    [PAL_COLOR_WHITE] = COLOR(0xfe, 0xfe, 0xfe),
};

void ES_SetPalette(const SDL_Surface* buffer) {
    SDL_Palette* palette = buffer->format->palette;
    const int first_color = PAL_COLOR_BLACK;
    const int num_colors = PAL_COLOR_NUM;
    SDL_SetPaletteColors(palette, ega_colors, first_color, num_colors);
}
