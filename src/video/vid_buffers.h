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

#ifndef __VID_BUFFERS__
#define __VID_BUFFERS__

void VID_SetPalette(const byte* palette);

void VID_ShiftPalette(const byte* palette);

void VID_ReallocBuffers(void);

void VID_FreeBuffers(void);

void VID_LockBuffer(void);

void VID_UnlockBuffer(void);

void VID_UpdateTexture(SDL_Texture* texture, vrect_t* rect);

#endif
