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


#include "sys.h"
#include "client.h"
#include "cmd.h"
#include "end_screen.h"
#include "input.h"
#include "keys.h"
#include "menu.h"
#include <SDL_events.h>
#include <SDL_hints.h>
#include <SDL_stdinc.h>
#include <SDL_timer.h>
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif


qboolean isDedicated;

/*
===============================================================================

FILE IO

===============================================================================
*/

#define MAX_HANDLES 10
FILE* sys_handles[MAX_HANDLES];

int findhandle(void) {
    int i;

    for (i = 1; i < MAX_HANDLES; i++)
        if (!sys_handles[i])
            return i;
    Sys_Error("out of handles");
    return -1;
}

/*
================
filelength
================
*/
int filelength(FILE* f) {
    int pos;
    int end;

    pos = ftell(f);
    fseek(f, 0, SEEK_END);
    end = ftell(f);
    fseek(f, pos, SEEK_SET);

    return end;
}

int Sys_FileOpenRead(char* path, int* hndl) {
    FILE* f;
    int i;

    i = findhandle();

    f = fopen(path, "rb");
    if (!f) {
        *hndl = -1;
        return -1;
    }
    sys_handles[i] = f;
    *hndl = i;

    return filelength(f);
}

int Sys_FileOpenWrite(char* path) {
    FILE* f;
    int i;

    i = findhandle();

    f = fopen(path, "wb");
    if (!f)
        Sys_Error("Error opening %s: %s", path, strerror(errno));
    sys_handles[i] = f;

    return i;
}

void Sys_FileClose(int handle) {
    fclose(sys_handles[handle]);
    sys_handles[handle] = NULL;
}

void Sys_FileSeek(int handle, int position) {
    fseek(sys_handles[handle], position, SEEK_SET);
}

int Sys_FileRead(int handle, void* dest, int count) {
    return fread(dest, 1, count, sys_handles[handle]);
}

int Sys_FileWrite(int handle, void* data, int count) {
    return fwrite(data, 1, count, sys_handles[handle]);
}

int Sys_FileTime(char* path) {
    FILE* f;

    f = fopen(path, "rb");
    if (f) {
        fclose(f);
        return 1;
    }

    return -1;
}

void Sys_mkdir(char* path) {
}


/*
===============================================================================

SYSTEM IO

===============================================================================
*/

void Sys_Error(char* error, ...) {
    va_list argptr;
    printf("Sys_Error: ");
    va_start(argptr, error);
    vprintf(error, argptr);
    va_end(argptr);
    printf("\n");

    Host_Shutdown();

    exit(1);
}

void Sys_Printf(char* fmt, ...) {
    va_list argptr;

    va_start(argptr, fmt);
    vprintf(fmt, argptr);
    va_end(argptr);
}

void Sys_Quit(void) {
    Host_Shutdown();
    ES_DisplayScreen();
    exit(0);
}

double Sys_FloatTime() {
    static double frequency = 0.0;
    static Uint64 start_time = 0;

    if (start_time == 0) {
        frequency = (double) SDL_GetPerformanceFrequency();
        start_time = SDL_GetPerformanceCounter();
        return (double) start_time / frequency;
    }
    Uint64 now = SDL_GetPerformanceCounter();
    double time_diff = (double) (now - start_time);
    return time_diff / frequency;
}

char* Sys_ConsoleInput(void) {
    return NULL;
}


/*
================================================================================

SYSTEM EVENT POLLING

================================================================================
*/

static void Sys_QuitEvent(void) {
    if (M_IsInQuitScreen()) {
        // Confirm quit.
        Key_Event('Y', true);
        return;
    }
    // Bring up the quit confirmation screen.
    Cmd_ExecuteString("quit", src_client);
}

void Sys_SendKeyEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_KEYDOWN:
            case SDL_KEYUP:
                IN_KeyboardEvent(&event);
                break;
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
            case SDL_MOUSEWHEEL:
                IN_MouseEvent(&event);
                break;
            case SDL_CONTROLLERDEVICEADDED:
            case SDL_CONTROLLERDEVICEREMOVED:
            case SDL_CONTROLLERBUTTONDOWN:
            case SDL_CONTROLLERBUTTONUP:
            case SDL_CONTROLLERAXISMOTION:
                IN_GamepadEvent(&event);
                break;
            case SDL_QUIT:
                Sys_QuitEvent();
                break;
            default:
                break;
        }
    }
}

//==============================================================================


void Sys_HighFPPrecision(void) {
}

void Sys_LowFPPrecision(void) {
}

//=============================================================================


/*
================================================================================

SIGNAL HANDLING

================================================================================
*/

#ifdef HAVE_SIGNAL_H
static void Sys_SigHandler(int sig) {
    CL_Disconnect();
    Host_ShutdownServer(false);
    Sys_Quit();
}

#ifdef HAVE_SIGACTION
static void Sys_SigHook(int sig) {
    struct sigaction action;
    sigaction(sig, NULL, &action);
    action.sa_handler = Sys_SigHandler;
    sigaction(sig, &action, NULL);
}
#else
static void Sys_SigHook(int sig) {
    signal(sig, Sys_SigHandler);
}
#endif

static void Sys_SigInit(void) {
    // Disable SDL default signal handlers
    SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
    Sys_SigHook(SIGINT);
    Sys_SigHook(SIGTERM);
}
#endif /* HAVE_SIGNAL_H */

//=============================================================================


#define DEFAULT_MEMORY (256 * 1024 * 1024)

static quakeparms_t* Sys_InitParms(int argc, char** argv) {
    static quakeparms_t parms;

    parms.memsize = DEFAULT_MEMORY;
    parms.membase = malloc(parms.memsize);
    parms.basedir = ".";

    COM_InitArgv(argc, argv);
    parms.argc = com_argc;
    parms.argv = com_argv;

    return &parms;
}

quakeparms_t* Sys_Init(int argc, char* argv[]) {
#ifdef HAVE_SIGNAL_H
    Sys_SigInit();
#endif
    return Sys_InitParms(argc, argv);
}
