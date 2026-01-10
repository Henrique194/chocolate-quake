/*
 * Copyright (C) 1996-1997 Id Software, Inc.
 * Copyright (C) Henrique Barateli, <henriquejb194@gmail.com>, et al.
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
// draw.h -- these are the only functions outside the refresh allowed
// to touch the vid buffer


#ifndef __DRAW__
#define __DRAW__

#include "quakedef.h"
#include "wad.h"

extern qpic_t* draw_disc; // also used on sbar

void Draw_Init(void);
void Draw_Character(i32 x, i32 y, i32 num);
void Draw_DebugChar(char num);
void Draw_Pic(i32 x, i32 y, qpic_t* pic);
void Draw_TransPic(i32 x, i32 y, qpic_t* pic);
void Draw_TransPicTranslate(i32 x, i32 y, qpic_t* pic, byte* translation);
void Draw_ConsoleBackground(i32 lines);
void Draw_BeginDisc(void);
void Draw_EndDisc(void);
void Draw_TileClear(i32 x, i32 y, i32 w, i32 h);
void Draw_Fill(i32 x, i32 y, i32 w, i32 h, i32 c);
void Draw_FadeScreen(void);
void Draw_String(i32 x, i32 y, char* str);
qpic_t* Draw_PicFromWad(char* name);
qpic_t* Draw_CachePic(char* path);

#endif
