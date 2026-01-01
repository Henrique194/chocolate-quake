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
// in_mouse.c -- mouse code


#include "in_mouse.h"
#include "host.h"
#include "keys.h"
#include <SDL_hints.h>


static qboolean mouse_active = true;


/*
================================================================================

MOUSE STATE

================================================================================
*/

void IN_ActivateMouse(void) {
    mouse_active = true;
}

void IN_DeactivateMouse(void) {
    mouse_active = false;
}

void IN_ShowMouse(void) {
    SDL_SetRelativeMouseMode(SDL_FALSE);
    SDL_ShowCursor(SDL_TRUE);
}

void IN_HideMouse(void) {
    // Relative mode for continuous mouse motion.
    SDL_SetRelativeMouseMode(SDL_TRUE);
    // Do not show cursor.
    SDL_ShowCursor(SDL_FALSE);
    // Zero mouse accumulation, so the camera doesn't go flying.
    SDL_GetRelativeMouseState(NULL, NULL);
}

//==============================================================================


/*
================================================================================

MOUSE EVENT

================================================================================
*/

static int IN_TranslateMouseButton(const Uint8 button) {
    switch (button) {
        case SDL_BUTTON_LEFT:
            return K_MOUSE1;
        case SDL_BUTTON_RIGHT:
            return K_MOUSE2;
        case SDL_BUTTON_MIDDLE:
            return K_MOUSE3;
        case SDL_BUTTON_X1:
            return K_MOUSE4;
        case SDL_BUTTON_X2:
            return K_MOUSE5;
        default:
            return 0;
    }
}

static void IN_ButtonEvent(const SDL_MouseButtonEvent* button) {
    const qboolean down = (button->state == SDL_PRESSED);
    const int key = IN_TranslateMouseButton(button->button);
    Key_Event(key, down);
}

static void IN_WheelEvent(const SDL_MouseWheelEvent* wheel) {
    if (wheel->y == 0) {
        return;
    }
    const int key = (wheel->y > 0 ? K_MWHEELUP : K_MWHEELDOWN);
    Key_Event(key, true);
    Key_Event(key, false);
}

void IN_MouseEvent(const SDL_Event* event) {
    switch (event->type) {
        case SDL_MOUSEBUTTONUP:
        case SDL_MOUSEBUTTONDOWN:
            IN_ButtonEvent(&event->button);
            break;
        case SDL_MOUSEWHEEL:
            IN_WheelEvent(&event->wheel);
            break;
        default:
            break;
    }
}

//==============================================================================


/*
================================================================================

MOUSE MOVE

================================================================================
*/

static qboolean IN_MouseLook(void) {
    return (in_mlook.state & 1) != 0;
}

static qboolean IN_LookStrafe(void) {
    return lookstrafe.value != 0;
}

static qboolean IN_StrafeActive(void) {
    return (in_strafe.state & 1) != 0;
}

static void IN_GetMouseMove(float* x, float* y) {
    int mx;
    int my;
    SDL_GetRelativeMouseState(&mx, &my);
    *x = (float) mx * sensitivity.value;
    *y = (float) my * sensitivity.value;
}

static void IN_AddHorizontalMove(usercmd_t* cmd, const float move_x) {
    if (IN_StrafeActive() || (IN_LookStrafe() && IN_MouseLook())) {
        cmd->sidemove += m_side.value * move_x;
        return;
    }
    cl.viewangles[YAW] -= m_yaw.value * move_x;
}

static void IN_AddVerticalMove(usercmd_t* cmd, const float move_y) {
    if (IN_MouseLook() && !IN_StrafeActive()) {
        cl.viewangles[PITCH] += (m_pitch.value * move_y);
        cl.viewangles[PITCH] = SDL_clamp(cl.viewangles[PITCH], -70, 80);
        return;
    }
    if (IN_StrafeActive() && noclip_anglehack) {
        cmd->upmove -= m_forward.value * move_y;
        return;
    }
    cmd->forwardmove -= m_forward.value * move_y;
}

void IN_MouseMove(usercmd_t* cmd) {
    if (!mouse_active) {
        return;
    }
    float move_x;
    float move_y;
    IN_GetMouseMove(&move_x, &move_y);
    IN_AddHorizontalMove(cmd, move_x);
    IN_AddVerticalMove(cmd, move_y);
    if (IN_MouseLook()) {
        V_StopPitchDrift();
    }
}

//==============================================================================


/*
================================================================================

INITIALIZATION

================================================================================
*/

void IN_InitMouse(void) {
    // Use system mouse acceleration.
    SDL_SetHint(SDL_HINT_MOUSE_RELATIVE_SYSTEM_SCALE, "1");
    IN_HideMouse();
}

//==============================================================================
