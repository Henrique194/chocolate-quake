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


#include "vid.h"
#include "sys.h"
#include "vid_buffers.h"
#include "vid_modes.h"
#include "vid_window.h"
#include <SDL.h>


//
// global video state
//
viddef_t vid;
static qboolean vid_initialized = false;

static byte backingbuf[48 * 24];

const Uint32 pixel_format = SDL_PIXELFORMAT_ARGB8888;


void VID_Init(const byte* palette) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        Sys_Error("Failed to initialize video: %s", SDL_GetError());
    }
    VID_InitWindow();
    VID_InitModes();
    VID_SetPalette(palette);
    vid_initialized = true;
}

void VID_Shutdown(void) {
    if (!vid_initialized) {
        return;
    }
    VID_ShutdownWindow();
    VID_FreeBuffers();
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    vid_initialized = false;
}

void VID_Update(vrect_t* rects) {
    VID_UpdateWindow(rects);
    VID_UpdateModes();
}

void D_BeginDirectRect(int x, int y, byte* pbitmap, int width, int height) {
    if (!vid_initialized) {
        return;
    }

    int repshift = (vid.aspect > 1.5f) ? 1 : 0;
    int reps = 1 << repshift;

    VID_LockBuffer();
    for (int i = 0; i < (height << repshift); i += reps) {
        for (int j = 0; j < reps; j++) {
            int spot_y = (y << repshift) + i + j;
            unsigned int spot = x + (spot_y * vid.width);

            // Save so later we can restore it.
            byte* dst = &backingbuf[(i + j) * 24];
            const byte* src = &vid.buffer[spot];
            memcpy(dst, src, width);

            dst = &vid.buffer[spot];
            src = &pbitmap[(i >> repshift) * width];
            memcpy(dst, src, width);
        }
    }
    VID_UnlockBuffer();

    vrect_t rect = {
        .x = x,
        .y = y,
        .width = width,
        .height = height << repshift,
    };
    VID_Update(&rect);
}

void D_EndDirectRect(int x, int y, int width, int height) {
    if (!vid_initialized) {
        return;
    }

    int repshift = (vid.aspect > 1.5f) ? 1 : 0;
    int reps = 1 << repshift;

    VID_LockBuffer();
    for (int i = 0; i < (height << repshift); i += reps) {
        for (int j = 0; j < reps; j++) {
            int spot_y = (y << repshift) + i + j;
            unsigned int spot = x + (spot_y * vid.width);

            // Restore from backup buffer.
            byte* dst = &vid.buffer[spot];
            const byte* src = &backingbuf[(i + j) * 24];

            memcpy(dst, src, width);
        }
    }
    VID_UnlockBuffer();

    vrect_t rect = {
        .x = x,
        .y = y,
        .width = width,
        .height = height << repshift,
    };
    VID_Update(&rect);
}
