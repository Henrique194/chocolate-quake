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
// vid_window.c -- handles window creation, management and resizing.


#include "quakedef.h"
#include "vid_window.h"
#include "vid_buffers.h"
#include "vid_modes.h"

static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;

// The intermediate texture that we load the RGBA buffer to.
static SDL_Texture* texture = NULL;

extern const Uint32 pixel_format;

static cvar_t _windowed_mouse = {"_windowed_mouse", "0", true};
static qboolean windowed_mouse;


/*
================================================================================

INITIALIZATION AND SHUTDOWN

================================================================================
*/

static void VID_CreateRenderer(void) {
    int index = -1;
    Uint32 flags = SDL_RENDERER_TARGETTEXTURE;
    renderer = SDL_CreateRenderer(window, index, flags);

    // Important: Set the "logical size" of the rendering context. At the same
    // time this also defines the aspect ratio that is preserved while scaling
    // and stretching the texture into the window.
    SDL_RenderSetLogicalSize(renderer, 320, 240);

    // Blank out the full screen area in case there is any junk in
    // the borders that won't otherwise be overwritten.
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
}

static void VID_CreateWindow(void) {
    int x = SDL_WINDOWPOS_CENTERED;
    int y = SDL_WINDOWPOS_CENTERED;
    int w = 0;
    int h = 0;
    Uint32 flags = SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_FULLSCREEN_DESKTOP;
    window = SDL_CreateWindow(PACKAGE_STRING, x, y, w, h, flags);
    if (window == NULL) {
        const char* error = SDL_GetError();
        Sys_Error("Error creating window for video startup: %s", error);
    }
    SDL_SetWindowMinimumSize(window, 320, 240);
}

static void VID_RegisterCvars(void) {
    Cvar_RegisterVariable(&_windowed_mouse);
    windowed_mouse = VID_WindowedMouse();
}

void VID_InitWindow(void) {
    VID_RegisterCvars();
    VID_CreateWindow();
    VID_CreateRenderer();
}

void VID_ShutdownWindow(void) {
    if (window) {
        SDL_DestroyWindow(window);
        window = NULL;
    }
    if (renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = NULL;
    }
    if (texture) {
        SDL_DestroyTexture(texture);
        texture = NULL;
    }
}

//==============================================================================


/*
================================================================================

MOUSE HANDLING

================================================================================
*/

static void VID_CenterMouse(void) {
    int x = (int) vid.width / 2;
    int y = (int) vid.height / 2;
    SDL_WarpMouseInWindow(window, x, y);
}

static void VID_ReleaseMouse(void) {
    VID_CenterMouse();
    IN_DeactivateMouse();
    IN_ShowMouse();
}

static void VID_GrabMouse(void) {
    IN_ActivateMouse();
    IN_HideMouse();
}

qboolean VID_WindowedMouse(void) {
    return (int) _windowed_mouse.value != 0;
}

void VID_ToggleMouseGrab(void) {
    windowed_mouse = !windowed_mouse;
    if (windowed_mouse) {
        VID_GrabMouse();
    } else {
        VID_ReleaseMouse();
    }
    Cvar_SetValue("_windowed_mouse", (float) windowed_mouse);
}

//==============================================================================


/*
================================================================================

VIDEO MODE CHANGE

================================================================================
*/

static void VID_SetWindowed(void) {
    int w = (int) vid.width;
    int h = (int) vid.height;
    int x = SDL_WINDOWPOS_CENTERED;
    int y = SDL_WINDOWPOS_CENTERED;
    SDL_SetWindowFullscreen(window, 0);
    SDL_SetWindowSize(window, w, h);
    SDL_SetWindowPosition(window, x, y);
    if (VID_WindowedMouse()) {
        VID_GrabMouse();
    } else {
        VID_ReleaseMouse();
    }
}

static void VID_SetFullscreen(void) {
    SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    VID_GrabMouse();
}

//
// Create the intermediate texture that the RGBA surface gets loaded into.
//
static void VID_AllocTexture(void) {
    int access = SDL_TEXTUREACCESS_STREAMING;
    int w = (int) vid.width;
    int h = (int) vid.height;
    texture = SDL_CreateTexture(renderer, pixel_format, access, w, h);
}

void VID_ResizeScreen(void) {
    if (texture) {
        SDL_DestroyTexture(texture);
        texture = NULL;
    }
    VID_AllocTexture();
    VID_ReallocBuffers();
    if (VID_IsFullscreenMode()) {
        VID_SetFullscreen();
    } else {
        VID_SetWindowed();
    }
}

//==============================================================================


static void VID_UpdateMouse(void) {
    if (VID_IsFullscreenMode()) {
        return;
    }
    if (windowed_mouse != VID_WindowedMouse()) {
        // Handle the mouse state when windowed if that's changed.
        VID_ToggleMouseGrab();
    }
}

static void VID_UpdateScreen(vrect_t* rect) {
    if (!rect) {
        return;
    }
    // Update the texture with the contents of the screen buffer.
    VID_UpdateTexture(texture, rect);
    // Clear the renderer's backbuffer to remove any previous contents.
    SDL_RenderClear(renderer);
    // Copy the updated texture to the backbuffer for rendering.
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    // Present the backbuffer content to the screen.
    SDL_RenderPresent(renderer);
}

void VID_UpdateWindow(vrect_t* rect) {
    VID_UpdateScreen(rect);
    VID_UpdateMouse();
}

void VID_HandlePause(qboolean pause) {
    if (VID_IsFullscreenMode()) {
        return;
    }
    if (pause) {
        VID_ReleaseMouse();
    } else {
        VID_GrabMouse();
    }
}

void VID_MinimizeWindow(void) {
    SDL_MinimizeWindow(window);
}
