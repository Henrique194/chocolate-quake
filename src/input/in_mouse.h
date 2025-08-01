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
// in_mouse.h -- mouse code


#ifndef __IN_MOUSE__
#define __IN_MOUSE__

void IN_ActivateMouse(void);

void IN_DeactivateMouse(void);

void IN_ShowMouse(void);

void IN_HideMouse(void);

void IN_MouseEvent(const SDL_Event* event);

void IN_MouseMove(usercmd_t* cmd);

void IN_InitMouse(void);

#endif
