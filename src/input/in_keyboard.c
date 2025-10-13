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
// in_keyboard.c -- keyboard code


#include "in_keyboard.h"
#include "keys.h"


static byte scancode_to_key[SDL_NUM_SCANCODES] = {
    [SDL_SCANCODE_A] = 'a',
    [SDL_SCANCODE_B] = 'b',
    [SDL_SCANCODE_C] = 'c',
    [SDL_SCANCODE_D] = 'd',
    [SDL_SCANCODE_E] = 'e',
    [SDL_SCANCODE_F] = 'f',
    [SDL_SCANCODE_G] = 'g',
    [SDL_SCANCODE_H] = 'h',
    [SDL_SCANCODE_I] = 'i',
    [SDL_SCANCODE_J] = 'j',
    [SDL_SCANCODE_K] = 'k',
    [SDL_SCANCODE_L] = 'l',
    [SDL_SCANCODE_M] = 'm',
    [SDL_SCANCODE_N] = 'n',
    [SDL_SCANCODE_O] = 'o',
    [SDL_SCANCODE_P] = 'p',
    [SDL_SCANCODE_Q] = 'q',
    [SDL_SCANCODE_R] = 'r',
    [SDL_SCANCODE_S] = 's',
    [SDL_SCANCODE_T] = 't',
    [SDL_SCANCODE_U] = 'u',
    [SDL_SCANCODE_V] = 'v',
    [SDL_SCANCODE_W] = 'w',
    [SDL_SCANCODE_X] = 'x',
    [SDL_SCANCODE_Y] = 'y',
    [SDL_SCANCODE_Z] = 'z',
    [SDL_SCANCODE_1] = '1',
    [SDL_SCANCODE_2] = '2',
    [SDL_SCANCODE_3] = '3',
    [SDL_SCANCODE_4] = '4',
    [SDL_SCANCODE_5] = '5',
    [SDL_SCANCODE_6] = '6',
    [SDL_SCANCODE_7] = '7',
    [SDL_SCANCODE_8] = '8',
    [SDL_SCANCODE_9] = '9',
    [SDL_SCANCODE_0] = '0',
    [SDL_SCANCODE_RETURN] = K_ENTER,
    [SDL_SCANCODE_ESCAPE] = K_ESCAPE,
    [SDL_SCANCODE_BACKSPACE] = K_BACKSPACE,
    [SDL_SCANCODE_TAB] = K_TAB,
    [SDL_SCANCODE_SPACE] = K_SPACE,
    [SDL_SCANCODE_MINUS] = '-',
    [SDL_SCANCODE_EQUALS] = '=',
    [SDL_SCANCODE_LEFTBRACKET] = '[',
    [SDL_SCANCODE_RIGHTBRACKET] = ']',
    [SDL_SCANCODE_BACKSLASH] = '\\',
    [SDL_SCANCODE_SEMICOLON] = ';',
    [SDL_SCANCODE_APOSTROPHE] = '\'',
    [SDL_SCANCODE_GRAVE] = '`',
    [SDL_SCANCODE_COMMA] = ',',
    [SDL_SCANCODE_PERIOD] = '.',
    [SDL_SCANCODE_SLASH] = '/',
    [SDL_SCANCODE_F1] = K_F1,
    [SDL_SCANCODE_F2] = K_F2,
    [SDL_SCANCODE_F3] = K_F3,
    [SDL_SCANCODE_F4] = K_F4,
    [SDL_SCANCODE_F5] = K_F5,
    [SDL_SCANCODE_F6] = K_F6,
    [SDL_SCANCODE_F7] = K_F7,
    [SDL_SCANCODE_F8] = K_F8,
    [SDL_SCANCODE_F9] = K_F9,
    [SDL_SCANCODE_F10] = K_F10,
    [SDL_SCANCODE_F11] = K_F11,
    [SDL_SCANCODE_F12] = K_F12,
    [SDL_SCANCODE_PAUSE] = K_PAUSE,
    [SDL_SCANCODE_INSERT] = K_INS,
    [SDL_SCANCODE_HOME] = K_HOME,
    [SDL_SCANCODE_PAGEUP] = K_PGUP,
    [SDL_SCANCODE_DELETE] = K_DEL,
    [SDL_SCANCODE_END] = K_END,
    [SDL_SCANCODE_PAGEDOWN] = K_PGDN,
    [SDL_SCANCODE_RIGHT] = K_RIGHTARROW,
    [SDL_SCANCODE_LEFT] = K_LEFTARROW,
    [SDL_SCANCODE_DOWN] = K_DOWNARROW,
    [SDL_SCANCODE_UP] = K_UPARROW,
    [SDL_SCANCODE_KP_DIVIDE] = '/',
    [SDL_SCANCODE_KP_MULTIPLY] = '*',
    [SDL_SCANCODE_KP_MINUS] = '-',
    [SDL_SCANCODE_KP_PLUS] = '+',
    [SDL_SCANCODE_KP_ENTER] = K_ENTER,
    [SDL_SCANCODE_KP_2] = K_DOWNARROW,
    [SDL_SCANCODE_KP_3] = K_PGDN,
    [SDL_SCANCODE_KP_4] = K_LEFTARROW,
    [SDL_SCANCODE_KP_5] = '5',
    [SDL_SCANCODE_KP_6] = K_RIGHTARROW,
    [SDL_SCANCODE_KP_8] = K_UPARROW,
    [SDL_SCANCODE_KP_PERIOD] = K_DEL,
    [SDL_SCANCODE_LCTRL] = K_CTRL,
    [SDL_SCANCODE_LSHIFT] = K_SHIFT,
    [SDL_SCANCODE_LALT] = K_ALT,
    [SDL_SCANCODE_RCTRL] = K_CTRL,
    [SDL_SCANCODE_RSHIFT] = K_SHIFT,
    [SDL_SCANCODE_RALT] = K_ALT,
};


/*
================================================================================

KEYBOARD EVENT

================================================================================
*/

static void IN_ButtonEvent(const SDL_KeyboardEvent* event) {
    const qboolean down = (event->state == SDL_PRESSED);
    const byte key = scancode_to_key[event->keysym.scancode];
    Key_Event(key, down);
}

void IN_KeyboardEvent(const SDL_Event* event) {
    switch (event->type) {
        case SDL_KEYDOWN:
        case SDL_KEYUP:
            IN_ButtonEvent(&event->key);
            break;
        default:
            break;
    }
}

//==============================================================================
