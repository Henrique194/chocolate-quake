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

#ifndef __VID_MODES__
#define __VID_MODES__

#include "quakedef.h"

#define NUM_WINDOWED_MODES   3
#define NUM_FULLSCREEN_MODES 12
#define NUM_MODES            (NUM_WINDOWED_MODES + NUM_FULLSCREEN_MODES)

#define DEFAULT_TEST_TIME 5

typedef enum {
    VID_MODE_WINDOWED,
    VID_MODE_FULLSCREEN,
} vid_mode_type_t;

typedef struct {
    const int width;
    const int height;
    const vid_mode_type_t type;
    const char* description;
    const char* full_description;
} vid_mode_t;

void VID_InitModes(void);

const vid_mode_t* VID_GetMode(int mode);

const vid_mode_t* VID_GetCurrentMode(void);

const vid_mode_t* VID_GetDefaultMode(void);

qboolean VID_IsFullscreenMode(void);

qboolean VID_IsWindowedMode(void);

qboolean VID_IsInTestMode(void);

void VID_SetMode(int mode_num);

void VID_SetCurrentModeAsDefault(void);

void VID_TestMode(int mode_num);

void VID_UpdateModes(void);

#endif
