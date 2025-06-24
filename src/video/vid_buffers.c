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
// vid_buffers.c -- video buffers for screen drawing


#include "quakedef.h"
#include "vid_buffers.h"
#include "d_local.h"


// The paletted buffer that we draw to (i.e. the one that holds vid_buffer).
static SDL_Surface* screen_buffer = NULL;

// The RGBA intermediate buffer that we blit the screen_buffer to.
static SDL_Surface* argb_buffer = NULL;

extern const Uint32 pixel_format;

static int VID_highhunkmark;
static int vid_surfcachesize;

static qboolean palette_changed;
static SDL_Color pal[256];


/*
================================================================================

PALETTE

================================================================================
*/

static void VID_UpdatePalette(void) {
    SDL_Palette* sdl_palette = screen_buffer->format->palette;
    SDL_SetPaletteColors(sdl_palette, pal, 0, 256);
    palette_changed = false;
}

void VID_SetPalette(const byte* palette) {
    // Translate the palette values to an SDL palette array and set the values.
    for (int i = 0; i < 256; i++) {
        byte r = palette[i * 3];
        byte g = palette[(i * 3) + 1];
        byte b = palette[(i * 3) + 2];

        // Zero out the bottom two bits of each channel:
        // the PC VGA controller only supports 6 bits of accuracy.
        pal[i].r = r & ~3;
        pal[i].g = g & ~3;
        pal[i].b = b & ~3;
        pal[i].a = SDL_ALPHA_OPAQUE;
    }

    palette_changed = true;
}

void VID_ShiftPalette(const byte* palette) {
    VID_SetPalette(palette);
}

//==============================================================================


/*
================================================================================

INITIALIZATION AND SHUTDOWN

================================================================================
*/

static void VID_AllocSurfaceCache() {
    byte* buffer = (byte*) d_pzbuffer;
    size_t cache_offset = vid.width * vid.height * sizeof(*d_pzbuffer);
    byte* cache = &buffer[cache_offset];
    D_InitCaches(cache, vid_surfcachesize);
}

static void VID_AllocZBuffer() {
    int chunk = vid.width * vid.height * sizeof(*d_pzbuffer);
    chunk += vid_surfcachesize;
    VID_highhunkmark = Hunk_HighMark();
    d_pzbuffer = Hunk_HighAllocName(chunk, "video");
    if (!d_pzbuffer) {
        Sys_Error("Not enough memory for video mode\n");
    }
}

//
// Create the 32-bit RGBA buffer. Format of argb_buffer must match the
// screen pixel format because we import the surface data into the texture.
//
static void VID_AllocRgbaBuffer(void) {
    int w = (int) vid.width;
    int h = (int) vid.height;
    int depth = 0;
    int pitch = 0;
    argb_buffer = SDL_CreateRGBSurfaceWithFormatFrom(NULL, w, h, depth, pitch,
                                                     pixel_format);

    SDL_FillRect(argb_buffer, NULL, 0);
}

//
// Create the 8-bit paletted screen buffer.
//
static void VID_AllocScreenBuffer(void) {
    Uint32 flags = 0;
    int w = (int) vid.width;
    int h = (int) vid.height;
    int depth = 8;
    Uint32 r_mask = 0;
    Uint32 g_mask = 0;
    Uint32 b_mask = 0;
    Uint32 a_mask = 0;

    screen_buffer = SDL_CreateRGBSurface(flags, w, h, depth, r_mask, g_mask,
                                         b_mask, a_mask);
    SDL_FillRect(screen_buffer, NULL, 0);

    vid.buffer = (byte*) screen_buffer->pixels;
}

void VID_ReallocBuffers(void) {
    VID_FreeBuffers();

    vid_surfcachesize = D_SurfaceCacheForRes(vid.width, vid.height);
    VID_AllocScreenBuffer();
    VID_AllocRgbaBuffer();
    VID_AllocZBuffer();
    VID_AllocSurfaceCache();

    VID_UpdatePalette();
}

void VID_FreeBuffers(void) {
    if (screen_buffer) {
        SDL_FreeSurface(screen_buffer);
        screen_buffer = NULL;
    }
    if (argb_buffer) {
        SDL_FreeSurface(argb_buffer);
        argb_buffer = NULL;
    }
    if (d_pzbuffer) {
        D_FlushCaches();
        Hunk_FreeToHighMark(VID_highhunkmark);
        d_pzbuffer = NULL;
    }
}

//==============================================================================


/*
================================================================================

BUFFER ACCESS

================================================================================
*/

void VID_LockBuffer(void) {
    if (SDL_MUSTLOCK(screen_buffer)) {
        SDL_LockSurface(screen_buffer);
    }
}

void VID_UnlockBuffer(void) {
    SDL_UnlockSurface(screen_buffer);
}

//
// Blit from the paletted 8-bit screen buffer to the intermediate
// 32-bit RGBA buffer and then update the intermediate texture with
// the contents of the RGBA buffer.
//
void VID_UpdateTexture(SDL_Texture* texture, vrect_t* rect) {
    if (palette_changed) {
        VID_UpdatePalette();
        // Ensure we blit the whole screen after updating the palette.
        rect->x = 0;
        rect->x = 0;
        rect->width = (int) vid.width;
        rect->height = (int) vid.height;
    }
    SDL_Rect src_rect = {
        .x = rect->x,
        .y = rect->y,
        .w = rect->width,
        .h = rect->height,
    };
    SDL_Rect dst_rect = {
        .x = 0,
        .y = 0,
        .w = rect->width,
        .h = rect->height,
    };
    SDL_LockTexture(texture, &src_rect, &argb_buffer->pixels,
                    &argb_buffer->pitch);
    SDL_LowerBlit(screen_buffer, &src_rect, argb_buffer, &dst_rect);
    SDL_UnlockTexture(texture);
}

//==============================================================================

