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
// vid_sdl.c -- SDL video routines


#include "quakedef.h"
#include "d_local.h"


viddef_t vid; // global video state

#define SCREEN_WIDTH      320
#define SCREEN_HEIGHT     200
#define SCREEN_HEIGHT_4_3 240

static byte* vid_buffer = NULL;
static byte backingbuf[48 * 24];
static short zbuffer[SCREEN_WIDTH * SCREEN_HEIGHT];
static byte* surfcache = NULL;
static size_t surfcache_size = 0;

static const Uint32 pixel_format = SDL_PIXELFORMAT_ARGB8888;
static qboolean palette_changed = false;
static qboolean vid_initialized = false;

static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;

// The 320x200x8 paletted buffer that we draw to
// (i.e. the one that holds vid_buffer).
static SDL_Surface* screen_buffer = NULL;

// The 320x200x32 RGBA intermediate buffer that we blit the screen_buffer to.
static SDL_Surface* argb_buffer = NULL;

// The intermediate 320x200 texture that we load the RGBA buffer to.
static SDL_Texture* texture = NULL;

static SDL_Color pal[256];


/*
================================================================================

PALETTE

================================================================================
*/

void VID_UpdatePalette(void) {
    SDL_Palette* sdl_palette = screen_buffer->format->palette;
    SDL_SetPaletteColors(sdl_palette, pal, 0, SDL_arraysize(pal));
    palette_changed = false;
}

void VID_SetPalette(unsigned char* palette) {
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

void VID_ShiftPalette(unsigned char* palette) {
    VID_SetPalette(palette);
}

//==============================================================================


/*
================================================================================

INITIALIZATION AND SHUTDOWN

================================================================================
*/

static void VID_InitCache(void) {
    surfcache_size = D_SurfaceCacheForRes(SCREEN_WIDTH, SCREEN_HEIGHT);
    surfcache = malloc(surfcache_size);
    D_InitCaches(surfcache, (int) surfcache_size);
}

static void VID_InitZBuffer(void) {
    d_pzbuffer = zbuffer;
}

static void VID_InitState(void) {
    vid.width = SCREEN_WIDTH;
    vid.maxwarpwidth = (int) vid.width;
    vid.conwidth = vid.width;

    vid.height = SCREEN_HEIGHT;
    vid.maxwarpheight = (int) vid.height;
    vid.conheight = vid.height;

    vid.rowbytes = SCREEN_WIDTH;
    vid.conrowbytes = SCREEN_WIDTH;

    vid.aspect = ((float) vid.height / (float) vid.width) * (320.0f / 240.0f);
    vid.numpages = 1;

    vid.colormap = host_colormap;
    vid.fullbright = 256 - LittleLong(*((int*) vid.colormap + 2048));

    vid.buffer = vid_buffer;
    vid.conbuffer = vid_buffer;
    vid.direct = vid_buffer;
}

//
// Create the intermediate texture that the RGBA surface gets loaded into.
//
static void VID_InitTexture(void) {
    // The SDL_TEXTUREACCESS_STREAMING flag means that this
    // texture's content is going to change frequently.
    int access = SDL_TEXTUREACCESS_STREAMING;
    int w = SCREEN_WIDTH;
    int h = SCREEN_HEIGHT;
    texture = SDL_CreateTexture(renderer, pixel_format, access, w, h);
}

//
// Create the 32-bit RGBA surface. Format of argb_buffer must match the
// screen pixel format because we import the surface data into the texture.
//
static void VID_Init32BitSurface(void) {
    int w = SCREEN_WIDTH;
    int h = SCREEN_HEIGHT;
    int depth = 0;
    int pitch = 0;
    argb_buffer = SDL_CreateRGBSurfaceWithFormatFrom(NULL, w, h, depth, pitch,
                                                     pixel_format);

    SDL_FillRect(argb_buffer, NULL, 0);
}

//
// Create the 8-bit paletted surface.
//
static void VID_Init8BitSurface(void) {
    Uint32 flags = 0;
    int w = 320;
    int h = SCREEN_HEIGHT;
    int depth = 8;
    Uint32 r_mask = 0;
    Uint32 g_mask = 0;
    Uint32 b_mask = 0;
    Uint32 a_mask = 0;

    screen_buffer = SDL_CreateRGBSurface(flags, w, h, depth, r_mask, g_mask,
                                         b_mask, a_mask);
    SDL_FillRect(screen_buffer, NULL, 0);

    vid_buffer = (byte*) screen_buffer->pixels;
}

static void VID_InitRenderer(void) {
    int index = -1;
    Uint32 flags = SDL_RENDERER_TARGETTEXTURE;
    renderer = SDL_CreateRenderer(window, index, flags);

    // Important: Set the "logical size" of the rendering context. At the same
    // time this also defines the aspect ratio that is preserved while scaling
    // and stretching the texture into the window.
    SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT_4_3);

    // Blank out the full screen area in case there is any junk in
    // the borders that won't otherwise be overwritten.
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
}

static void VID_InitWindow(void) {
    int x = SDL_WINDOWPOS_CENTERED;
    int y = SDL_WINDOWPOS_CENTERED;
    int w = 0;
    int h = 0;
    Uint32 flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI |
                   SDL_WINDOW_FULLSCREEN_DESKTOP;
    window = SDL_CreateWindow("Chocolate Quake", x, y, w, h, flags);
    if (window == NULL) {
        const char* error = SDL_GetError();
        Sys_Error("Error creating window for video startup: %s", error);
    }
    SDL_SetWindowMinimumSize(window, SCREEN_WIDTH, SCREEN_HEIGHT_4_3);
}

void VID_Init(unsigned char* palette) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        Sys_Error("Failed to initialize video: %s", SDL_GetError());
    }
    VID_InitWindow();
    VID_InitRenderer();
    VID_Init8BitSurface();
    VID_Init32BitSurface();
    VID_InitTexture();
    VID_InitState();
    VID_InitZBuffer();
    VID_InitCache();
    VID_SetPalette(palette);
    vid_initialized = true;
}

void VID_Shutdown(void) {
    if (!vid_initialized) {
        return;
    }
    if (surfcache) {
        free(surfcache);
        surfcache = NULL;
    }
    if (window) {
        SDL_DestroyWindow(window);
        window = NULL;
    }
    if (renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = NULL;
    }
    if (screen_buffer) {
        SDL_FreeSurface(screen_buffer);
        screen_buffer = NULL;
    }
    if (argb_buffer) {
        SDL_FreeSurface(argb_buffer);
        argb_buffer = NULL;
    }
    if (texture) {
        SDL_DestroyTexture(texture);
        texture = NULL;
    }
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    vid_initialized = false;
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
// Update the screen with any rendering performed since the previous call.
//
static void VID_Present(void) {
    // Make sure the pillarboxes are kept clear each frame.
    SDL_RenderClear(renderer);

    SDL_RenderCopy(renderer, texture, NULL, NULL);

    // Draw!
    SDL_RenderPresent(renderer);
}

//
// Blit from the paletted 8-bit screen buffer to the intermediate
// 32-bit RGBA buffer and update the intermediate texture with the
// contents of the RGBA buffer.
//
static void VID_UpdateTexture(vrect_t* rects) {
    SDL_LockTexture(texture, NULL, &argb_buffer->pixels, &argb_buffer->pitch);
    while (rects) {
        SDL_Rect sdl_rect = {
            .x = rects->x,
            .y = rects->y,
            .w = rects->width,
            .h = rects->height,
        };
        SDL_LowerBlit(screen_buffer, &sdl_rect, argb_buffer, &sdl_rect);
        rects = rects->pnext;
    }
    SDL_UnlockTexture(texture);
}

void VID_Update(vrect_t* rects) {
    if (!rects) {
        return;
    }

    vrect_t rect;
    if (palette_changed) {
        VID_UpdatePalette();
        // Ensure we blit the whole screen after updating the palette.
        rect.x = 0;
        rect.y = 0;
        rect.width = (int) vid.width;
        rect.height = (int) vid.height;
        rect.pnext = NULL;
        rects = &rect;
    }

    VID_UpdateTexture(rects);
    VID_Present();
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
            unsigned int spot = x + (spot_y * vid.rowbytes);

            // Save so later we can restore it.
            byte* dst = &backingbuf[(i + j) * 24];
            const byte* src = &vid.direct[spot];
            memcpy(dst, src, width);

            dst = &vid.direct[spot];
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
        .pnext = NULL,
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
            unsigned int spot = x + (spot_y * vid.rowbytes);

            // Restore from backup buffer.
            byte* dst = &vid.direct[spot];
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
        .pnext = NULL,
    };
    VID_Update(&rect);
}

//==============================================================================
