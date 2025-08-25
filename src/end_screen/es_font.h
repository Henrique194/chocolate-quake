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


#ifndef __ES_FONT__
#define __ES_FONT__

typedef enum {
    FONT_SMALL,
    FONT_NORMAL,
    FONT_NORMAL_HIGHDPI,
    FONT_LARGE,
    FONT_NUM,
} font_type_t;

typedef struct {
    const byte* data;
    unsigned int w;
    unsigned int h;
} font_t;

void ES_ChooseFont(void);

void ES_SetFont(font_type_t type);

const font_t* ES_GetFont(font_type_t type);

const font_t* ES_GetCurrentFont(void);

qboolean ES_IsHighDPIFont(void);

#endif
