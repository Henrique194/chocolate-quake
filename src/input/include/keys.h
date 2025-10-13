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


#ifndef __KEYS__
#define __KEYS__

#include "quakedef.h"
#include <stdio.h>

//
// these are the key numbers that should be passed to Key_Event
//
#define K_TAB    9
#define K_ENTER  13
#define K_ESCAPE 27
#define K_SPACE  32

// normal keys should be passed as lowercased ascii

#define K_BACKSPACE  127
#define K_UPARROW    128
#define K_DOWNARROW  129
#define K_LEFTARROW  130
#define K_RIGHTARROW 131

#define K_ALT   132
#define K_CTRL  133
#define K_SHIFT 134
#define K_F1    135
#define K_F2    136
#define K_F3    137
#define K_F4    138
#define K_F5    139
#define K_F6    140
#define K_F7    141
#define K_F8    142
#define K_F9    143
#define K_F10   144
#define K_F11   145
#define K_F12   146
#define K_INS   147
#define K_DEL   148
#define K_PGDN  149
#define K_PGUP  150
#define K_HOME  151
#define K_END   152

#define K_PAUSE 255

//
// mouse buttons generate virtual keys
//
#define K_MOUSE1 200
#define K_MOUSE2 201
#define K_MOUSE3 202
#define K_MOUSE4 203
#define K_MOUSE5 204

//
// gamepad buttons
//
#define K_ABUTTON   205
#define K_BBUTTON   206
#define K_XBUTTON   207
#define K_YBUTTON   208
#define K_LTHUMB    209
#define K_RTHUMB    210
#define K_LSHOULDER 211
#define K_RSHOULDER 212
#define K_LTRIGGER  213
#define K_RTRIGGER  214

//
// aux keys are for multi-buttoned joysticks to generate so they can use
// the normal binding process
//
#define K_AUX1  215
#define K_AUX2  216
#define K_AUX3  217
#define K_AUX4  218
#define K_AUX5  219
#define K_AUX6  220
#define K_AUX7  221
#define K_AUX8  222
#define K_AUX9  223
#define K_AUX10 224
#define K_AUX11 225
#define K_AUX12 226
#define K_AUX13 227
#define K_AUX14 228
#define K_AUX15 229
#define K_AUX16 230
#define K_AUX17 231
#define K_AUX18 232
#define K_AUX19 233
#define K_AUX20 234
#define K_AUX21 235
#define K_AUX22 236
#define K_AUX23 237
#define K_AUX24 238
#define K_AUX25 239
#define K_AUX26 240

// JACK: Intellimouse(c) Mouse Wheel Support

#define K_MWHEELUP   241
#define K_MWHEELDOWN 242

typedef enum {
    key_game,
    key_console,
    key_message,
    key_menu,
} keydest_t;

extern keydest_t key_dest;
extern char* keybindings[256];
extern int key_repeats[256];
extern int key_count; // incremented every key event
extern int key_lastpress;

void Key_Event(int key, qboolean down);
void Key_Init(void);
void Key_WriteBindings(FILE* f);
void Key_SetBinding(int keynum, char* binding);
void Key_ClearStates(void);

#endif