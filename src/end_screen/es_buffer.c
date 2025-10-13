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


#include "es_buffer.h"
#include "cvar.h"
#include "es_palette.h"
#include "es_font.h"
#include "es_time.h"
#include <SDL_hints.h>
#include <SDL_timer.h>

typedef struct {
    byte ch;
    byte fg_color;
    byte bg_color;
    qboolean blink;
} vga_char_t;

static SDL_Surface* screen_buffer;
static SDL_Surface* argb_buffer;
static SDL_Texture* texture;
static vga_char_t end_screen[TEXT_SCREEN_SIZE];

static int buffer_width;
static int buffer_height;

static const SDL_PixelFormatEnum pixel_format = SDL_PIXELFORMAT_ARGB8888;

static qboolean has_blink_char = false;


/*
================================================================================

INITIALIZATION AND SHUTDOWN

================================================================================
*/

static byte* ES_GetScreenData(void) {
    char* file = (registered.value != 0) ? "end2.bin" : "end1.bin";
    byte* screen_data = COM_LoadHunkFile(file);
    if (!screen_data) {
        return NULL;
    }

    // Write the version number directly to the end screen.
    char ver[7];
    SDL_snprintf(ver, 7, " v%4.2f", VERSION);
    for (int i = 0; i < 6; i++) {
        screen_data[(72 + i) * 2] = ver[i];
    }

    return screen_data;
}

static qboolean ES_ReadScreenData(void) {
    const byte* screen_data = ES_GetScreenData();
    if (!screen_data) {
        return false;
    }
    const int size = TEXT_SCREEN_SIZE * 2;
    for (int i = 0; i < size; i += 2) {
        const byte char_byte = screen_data[i];
        const byte color_byte = screen_data[i + 1];

        vga_char_t* vga_char = &end_screen[i / 2];
        vga_char->ch = char_byte;
        vga_char->fg_color = color_byte & 0xf;
        vga_char->bg_color = (color_byte >> 4) & 0x7;
        vga_char->blink = color_byte >> 7;

        if (vga_char->blink) {
            has_blink_char = true;
        }
    }
    return true;
}

static qboolean ES_CreateScreenBuffer(void) {
    const int w = buffer_width;
    const int h = buffer_height;
    const Uint32 flags = 0;
    const int depth = 8;
    const Uint32 r = 0;
    const Uint32 g = 0;
    const Uint32 b = 0;
    const Uint32 a = 0;
    screen_buffer = SDL_CreateRGBSurface(flags, w, h, depth, r, g, b, a);
    if (!screen_buffer) {
        return false;
    }
    ES_SetPalette(screen_buffer);
    return true;
}

static SDL_bool ES_CreateRgbaBuffer(void) {
    const int w = buffer_width;
    const int h = buffer_height;
    const int depth = 0;
    const int pitch = 0;
    argb_buffer = SDL_CreateRGBSurfaceWithFormatFrom(NULL, w, h, depth, pitch,
                                                     pixel_format);
    if (!argb_buffer) {
        return SDL_FALSE;
    }
    SDL_FillRect(argb_buffer, NULL, 0);
    return SDL_TRUE;
}

static SDL_bool ES_CreateTexture(SDL_Renderer* renderer) {
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

    const int w = buffer_width;
    const int h = buffer_height;
    const int access = SDL_TEXTUREACCESS_STREAMING;
    texture = SDL_CreateTexture(renderer, pixel_format, access, w, h);
    return texture != NULL;
}

qboolean ES_InitBuffers(SDL_Renderer* renderer) {
    const font_t* font = ES_GetCurrentFont();
    buffer_width = TEXT_SCREEN_WIDTH * font->w;
    buffer_height = TEXT_SCREEN_HEIGHT * font->h;
    if (!ES_ReadScreenData()) {
        return false;
    }
    if (!ES_CreateTexture(renderer)) {
        return false;
    }
    if (!ES_CreateRgbaBuffer()) {
        return false;
    }
    if (!ES_CreateScreenBuffer()) {
        return false;
    }
    return true;
}

void ES_FreeBuffers(void) {
    if (texture) {
        SDL_DestroyTexture(texture);
        texture = NULL;
    }
    if (argb_buffer) {
        SDL_FreeSurface(argb_buffer);
        argb_buffer = NULL;
    }
    if (screen_buffer) {
        SDL_FreeSurface(screen_buffer);
        screen_buffer = NULL;
    }
}

//==============================================================================


/*
================================================================================

SCREEN UPDATE

================================================================================
*/

static void ES_UpdateTexture(void) {
    SDL_Rect rect = {
        .x = 0,
        .y = 0,
        .w = buffer_width,
        .h = buffer_height,
    };
    SDL_LockTexture(texture, &rect, &argb_buffer->pixels, &argb_buffer->pitch);
    SDL_LowerBlit(screen_buffer, &rect, argb_buffer, &rect);
    SDL_UnlockTexture(texture);
}

static void ES_UpdateCharacter(const int x, const int y) {
    // Retrieve VGA char.
    const vga_char_t* vga_char = &end_screen[x + (y * TEXT_SCREEN_WIDTH)];
    const byte character = vga_char->ch;
    const byte bg = vga_char->bg_color;
    byte fg = vga_char->fg_color;
    if (vga_char->blink && ((SDL_GetTicks() / BLINK_PERIOD) % 2) == 0) {
        fg = bg;
    }

    // Retrieve char data from font.
    const font_t* font = ES_GetCurrentFont();
    const Uint32 char_size = (font->w * font->h) / 8;
    const byte* char_data = &font->data[character * char_size];

    const unsigned int spot = (x * font->w) + (y * font->h * screen_buffer->pitch);
    byte* dst = ((byte*) screen_buffer->pixels) + spot;
    byte bit = 0;

    for (int y1 = 0; y1 < font->h; y1++) {
        for (int x1 = 0; x1 < font->w; x1++) {
            dst[x1] = (*char_data & (1 << bit)) ? fg : bg;
            bit++;
            if (bit == 8) {
                char_data++;
                bit = 0;
            }
        }
        dst += screen_buffer->pitch;
    }
}

static void ES_UpdateScreenBuffer(void) {
    SDL_LockSurface(screen_buffer);
    for (int y = 0; y < TEXT_SCREEN_HEIGHT; y++) {
        for (int x = 0; x < TEXT_SCREEN_WIDTH; x++) {
            ES_UpdateCharacter(x, y);
        }
    }
    SDL_UnlockSurface(screen_buffer);
}

void ES_UpdateScreen(SDL_Renderer* renderer) {
    ES_UpdateScreenBuffer();
    ES_UpdateTexture();
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
}

//==============================================================================


qboolean ES_HasBlinkChar(void) {
    return has_blink_char;
}