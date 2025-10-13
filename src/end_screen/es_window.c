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


#include "es_window.h"
#include "config.h"
#include "es_buffer.h"
#include "es_font.h"
#include <SDL.h>

static SDL_Window* window;
static SDL_Renderer* renderer;


/*
================================================================================

INITIALIZATION AND SHUTDOWN

================================================================================
*/


static qboolean ES_CreateRenderer(void) {
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    }
    return renderer != NULL;
}

static qboolean ES_CreateWindow(void) {
    const font_t* font = ES_GetCurrentFont();
    const int w = TEXT_SCREEN_WIDTH * font->w;
    const int h = TEXT_SCREEN_HEIGHT * font->h;
    const int x = SDL_WINDOWPOS_CENTERED;
    const int y = SDL_WINDOWPOS_CENTERED;
    const char* title = PACKAGE_STRING;
    const Uint32 flags = (ES_IsHighDPIFont() ? SDL_WINDOW_ALLOW_HIGHDPI : 0);
    window = SDL_CreateWindow(title, x, y, w, h, flags);
    return window != NULL;
}

int ES_InitWindow(void) {
    if (SDL_Init(SDL_INIT_VIDEO)) {
        return false;
    }
    if (!ES_CreateWindow()) {
        return false;
    }
    if (!ES_CreateRenderer()) {
        return false;
    }
    if (!ES_InitBuffers(renderer)) {
        return false;
    }
    return true;
}

void ES_ShutdownWindow(void) {
    ES_FreeBuffers();
    if (renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = NULL;
    }
    if (window) {
        SDL_DestroyWindow(window);
        window = NULL;
    }
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

//==============================================================================


void ES_RefreshWindow(void) {
    ES_UpdateScreen(renderer);
    SDL_RenderPresent(renderer);
}
