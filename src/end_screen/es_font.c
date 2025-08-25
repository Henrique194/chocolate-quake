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


#include "quakedef.h"
#include "es_font.h"
#include "fonts/large.h"
#include "fonts/normal.h"
#include "fonts/small.h"
#ifdef _WIN32
#include "windows.h"
#endif

static const font_t* font;
static font_type_t font_type;


static const char* ES_GetFontName(const font_type_t type) {
    switch (type) {
        case FONT_SMALL:
            return "small";
        case FONT_NORMAL:
            return "normal";
        case FONT_NORMAL_HIGHDPI:
            return "normal-highdpi";
        case FONT_LARGE:
            return "large";
        default:
            return NULL;
    }
}

static font_type_t ES_GetFontByName(const char* name) {
    for (int i = 0; i < FONT_NUM; i++) {
        const char* font_name = ES_GetFontName(i);
        if (!strcmp(name, font_name)) {
            return i;
        }
    }
    return -1;
}

const font_t* ES_GetFont(const font_type_t type) {
    switch (type) {
        case FONT_SMALL:
            return &small_font;
        case FONT_NORMAL:
        case FONT_NORMAL_HIGHDPI:
            // high dpi font usually means normal font (the normal resolution
            // version), but actually means "set the window high dpi flag and
            // try to use large font if we initialize successfully".
            return &normal_font;
        case FONT_LARGE:
            return &large_font;
        default:
            return NULL;
    }
}

const font_t* ES_GetCurrentFont(void) {
    return font;
}

void ES_SetFont(const font_type_t type) {
    const font_t* new_font = ES_GetFont(type);
    if (new_font) {
        font = new_font;
        font_type = type;
    }
}

qboolean ES_IsHighDPIFont(void) {
    return font_type == FONT_NORMAL_HIGHDPI;
}


#ifdef _WIN32
//
// On Windows we can use the system DPI settings to make a
// more educated guess about whether to use the large font.
//
static int ES_CanUseLargeFont(void) {
    HDC hdc = GetDC(NULL);
    if (!hdc) {
        return 0;
    }
    const int dpix = GetDeviceCaps(hdc, LOGPIXELSX);
    ReleaseDC(NULL, hdc);
    // 144 is the DPI when using "150%" scaling. If the user has this set
    // then consider this an appropriate threshold for using the large font.
    return dpix >= 144;
}
#endif

void ES_ChooseFont(void) {
    // Allow normal selection to be overridden from an environment variable:
    const char* env = SDL_getenv("CQ_ENDSCREEN_FONT");
    if (env) {
        const font_type_t type = ES_GetFontByName(env);
        if (type >= 0) {
            ES_SetFont(type);
            return;
        }
    }

    // Get desktop resolution.
    SDL_DisplayMode desktop_info;
    if (SDL_GetCurrentDisplayMode(0, &desktop_info)) {
        // If in doubt and we can't get a list, always prefer
        // to fall back to the normal font.
        ES_SetFont(FONT_NORMAL_HIGHDPI);
        return;
    }

    if (desktop_info.w < 640 || desktop_info.h < 480) {
        // On tiny low-res screens (eg. palmtops) use the small font.
        // If the screen resolution is at least 1920x1080, this is
        // a modern high-resolution display, and we can use the
        // large font.
        ES_SetFont(FONT_SMALL);
        return;
    }
#ifdef _WIN32
    if (ES_CanUseLargeFont()) {
        ES_SetFont(FONT_LARGE);
        return;
    }
#endif
    ES_SetFont(FONT_NORMAL_HIGHDPI);
}
