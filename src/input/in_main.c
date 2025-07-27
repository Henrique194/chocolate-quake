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
// in_main.c -- main input functions


#include "quakedef.h"
#include "in_mouse.h"
#include "in_gamepad.h"


void IN_Init(void) {
    IN_InitMouse();
    IN_InitGamepad();
}

void IN_Shutdown(void) {
    IN_ShutdownGamepad();
}

void IN_Move(usercmd_t* cmd) {
    IN_MouseMove(cmd);
    IN_JoyMove(cmd);
}
