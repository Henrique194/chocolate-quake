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
// in_sdl.c -- SDL mouse and joystick code


#include "quakedef.h"

static qboolean mouse_active = true;

void IN_Init(void) {
    // Relative mode for continuous mouse motion.
    SDL_SetRelativeMouseMode(SDL_TRUE);
    // Zero mouse accumulation when initiating game.
    SDL_GetRelativeMouseState(NULL, NULL);
    // Use system mouse acceleration.
    SDL_SetHint(SDL_HINT_MOUSE_RELATIVE_SYSTEM_SCALE, "1");
}

void IN_Shutdown(void) {
}

void IN_Commands(void) {
}

static int Sys_TranslateMouse(Uint8 button) {
    switch (button) {
        case 1:
            return K_MOUSE1;
        case 2:
            return K_MOUSE3;
        case 3:
            return K_MOUSE2;
        default:
            return -1;
    }
}

void IN_MouseEvent(const SDL_Event* event) {
    int down;
    int button;
    switch (event->type) {
        case SDL_MOUSEBUTTONUP:
        case SDL_MOUSEBUTTONDOWN:
            down = (event->button.state == SDL_PRESSED);
            button = Sys_TranslateMouse(event->button.button);
            if (button != -1) {
                Key_Event(button, down);
            }
            break;
        case SDL_MOUSEWHEEL:
            if (event->wheel.y > 0) {
                Key_Event(K_MWHEELUP, 1);
                Key_Event(K_MWHEELUP, 0);
            } else if (event->wheel.y < 0) {
                Key_Event(K_MWHEELDOWN, 1);
                Key_Event(K_MWHEELDOWN, 0);
            }
        default:
            break;
    }
}

void IN_MouseMove(usercmd_t* cmd) {
    int mx;
    int my;
    float mouse_x;
    float mouse_y;

    SDL_GetRelativeMouseState(&mx, &my);

    mouse_x = (float) mx;
    mouse_y = (float) my;

    mouse_x *= sensitivity.value;
    mouse_y *= sensitivity.value;

    // add mouse X/Y movement to cmd
    if ((in_strafe.state & 1) || (lookstrafe.value && (in_mlook.state & 1)))
        cmd->sidemove += m_side.value * mouse_x;
    else
        cl.viewangles[YAW] -= m_yaw.value * mouse_x;

    if (in_mlook.state & 1)
        V_StopPitchDrift();

    if ((in_mlook.state & 1) && !(in_strafe.state & 1)) {
        cl.viewangles[PITCH] += m_pitch.value * mouse_y;
        if (cl.viewangles[PITCH] > 80)
            cl.viewangles[PITCH] = 80;
        if (cl.viewangles[PITCH] < -70)
            cl.viewangles[PITCH] = -70;
    } else {
        if ((in_strafe.state & 1) && noclip_anglehack)
            cmd->upmove -= m_forward.value * mouse_y;
        else
            cmd->forwardmove -= m_forward.value * mouse_y;
    }
}

void IN_DeactivateMouse(void) {
    mouse_active = false;
}

void IN_ActivateMouse(void) {
    mouse_active = true;
}

void IN_ShowMouse(void) {
    SDL_SetRelativeMouseMode(SDL_FALSE);
    SDL_ShowCursor(SDL_TRUE);
}

void IN_HideMouse(void) {
    SDL_SetRelativeMouseMode(SDL_TRUE);
    SDL_ShowCursor(SDL_FALSE);
    // consume all pending mouse events, so the camera doesn't go flying
    SDL_GetRelativeMouseState(NULL, NULL);
}

void IN_JoyMove(usercmd_t* cmd) {
}

void IN_Move(usercmd_t* cmd) {
    if (mouse_active) {
        IN_MouseMove(cmd);
    }
    IN_JoyMove(cmd);
}
