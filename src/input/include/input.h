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
// input.h -- input devices


#ifndef __INPUT__
#define __INPUT__

void IN_Init(void);

void IN_Shutdown(void);

void IN_KeyboardEvent(const SDL_Event* event);

void IN_MouseEvent(const SDL_Event* event);

void IN_GamepadEvent(const SDL_Event* event);

// add additional movement on top of the keyboard move cmd
void IN_Move(usercmd_t* cmd);

void IN_DeactivateMouse(void);

void IN_ActivateMouse(void);

void IN_ShowMouse(void);

void IN_HideMouse(void);

// restores all button and position states to defaults
void IN_ClearStates(void);

#endif
