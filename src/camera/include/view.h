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
// view.h

#ifndef __VIEW__
#define __VIEW__

#include "quakedef.h"
#include "cvar.h"
#include "mathlib.h"

extern cvar_t v_gamma;

extern byte gammatable[256]; // palette is sent through this
extern byte ramps[3][256];
extern float v_blend[4];

extern cvar_t lcd_x;

extern cvar_t chase_active;


void V_Init(void);
void V_RenderView(void);
float V_CalcRoll(vec3_t angles, vec3_t velocity);
void V_UpdatePalette(void);

void Chase_Init(void);
void Chase_Reset(void);
void Chase_Update(void);

#endif
