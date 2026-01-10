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
// vid_modes.c -- manages screen resolution settings


#include "vid_modes.h"
#include "cmd.h"
#include "console.h"
#include "cvar.h"
#include "host.h"
#include "vid.h"
#include "vid_window.h"


#define VALID_MODE(mode) ((mode) >= 0 && (mode) < NUM_MODES)
#define IS_WINDOWED(mode) ((mode) >= 0 && (mode) < NUM_WINDOWED_MODES)

#define DEFAULT_WINDOWED_MODE 1

#define VID_MODE(w, h, mode_type, name)                                        \
    {                                                                          \
        .width = w,                                                            \
        .height = h,                                                           \
        .type = mode_type,                                                     \
        .description = #w "x" #h,                                              \
        .full_description = #w "x" #h " " #name                                \
    }

#define WINDOWED_MODE(w, h) VID_MODE(w, h, VID_MODE_WINDOWED, windowed)
#define FULLSCREEN_MODE(w, h) VID_MODE(w, h, VID_MODE_FULLSCREEN, fullscreen)


static vid_mode_t modes[NUM_MODES] = {
    WINDOWED_MODE(320, 240),
    WINDOWED_MODE(640, 480),
    WINDOWED_MODE(800, 600),
    FULLSCREEN_MODE(320, 200),
    FULLSCREEN_MODE(320, 240),
    FULLSCREEN_MODE(640, 400),
    FULLSCREEN_MODE(640, 480),
    FULLSCREEN_MODE(800, 600),
    FULLSCREEN_MODE(1024, 768),
    FULLSCREEN_MODE(1152, 864),
    FULLSCREEN_MODE(1280, 720),
    FULLSCREEN_MODE(1280, 768),
    FULLSCREEN_MODE(1280, 800),
    FULLSCREEN_MODE(1280, 960),
    FULLSCREEN_MODE(1280, 1024),
};

static i32 current_mode = 0;
static i32 previous_mode = 0;
static i32 default_mode = 0;

static cvar_t vid_mode = {"vid_mode", "0", false};
static cvar_t _vid_default_mode_win = {"_vid_default_mode_win", "3", true};
static cvar_t vid_windowed_mode = {"vid_windowed_mode", "0", true};
static cvar_t vid_fullscreen_mode = {"vid_fullscreen_mode", "3", true};

static qboolean testing_mode = false;
static double test_end_time = 0.0;

static qboolean first_update = true;
static qboolean force_set_mode = false;
static qboolean start_windowed = false;


/*
================================================================================

INITIALIZATION

================================================================================
*/

static void VID_TestMode_f(void);
static void VID_NumModes_f(void);
static void VID_DescribeCurrentMode_f(void);
static void VID_DescribeMode_f(void);
static void VID_DescribeModes_f(void);
static void VID_ForceMode_f(void);
static void VID_Windowed_f(void);
static void VID_Fullscreen_f(void);
static void VID_Minimize_f(void);

static void VID_SetInitialMode(void) {
    i32 init_mode;
    if (COM_CheckParm("-startwindowed")) {
        init_mode = DEFAULT_WINDOWED_MODE;
        start_windowed = true;
    } else {
        init_mode = default_mode;
    }
    VID_SetMode(init_mode);
}

static void VID_RegisterCommands(void) {
    Cmd_AddCommand("vid_testmode", VID_TestMode_f);
    Cmd_AddCommand("vid_nummodes", VID_NumModes_f);
    Cmd_AddCommand("vid_describecurrentmode", VID_DescribeCurrentMode_f);
    Cmd_AddCommand("vid_describemode", VID_DescribeMode_f);
    Cmd_AddCommand("vid_describemodes", VID_DescribeModes_f);
    Cmd_AddCommand("vid_forcemode", VID_ForceMode_f);
    Cmd_AddCommand("vid_windowed", VID_Windowed_f);
    Cmd_AddCommand("vid_fullscreen", VID_Fullscreen_f);
    Cmd_AddCommand("vid_minimize", VID_Minimize_f);
}

static void VID_RegisterCvars(void) {
    Cvar_RegisterVariable(&vid_mode);
    Cvar_RegisterVariable(&_vid_default_mode_win);
    Cvar_RegisterVariable(&vid_windowed_mode);
    Cvar_RegisterVariable(&vid_fullscreen_mode);
    default_mode = (i32) _vid_default_mode_win.value;
}

void VID_InitModes(void) {
    VID_RegisterCvars();
    VID_RegisterCommands();
    VID_SetInitialMode();
}

//==============================================================================


/*
================================================================================

QUERY MODE INFORMATION

================================================================================
*/

const vid_mode_t* VID_GetMode(i32 mode_num) {
    if (!VALID_MODE(mode_num)) {
        return NULL;
    }
    return &modes[mode_num];
}

const vid_mode_t* VID_GetCurrentMode(void) {
    return VID_GetMode(current_mode);
}

const vid_mode_t* VID_GetDefaultMode(void) {
    return VID_GetMode(default_mode);
}

qboolean VID_IsFullscreenMode(void) {
    const vid_mode_t* mode = VID_GetCurrentMode();
    return mode->type == VID_MODE_FULLSCREEN;
}

qboolean VID_IsWindowedMode(void) {
    const vid_mode_t* mode = VID_GetCurrentMode();
    return mode->type == VID_MODE_WINDOWED;
}

qboolean VID_IsInTestMode(void) {
    return testing_mode;
}

//==============================================================================


/*
================================================================================

CHANGE MODE

================================================================================
*/

static void VID_ApplyMode(i32 mode_num) {
    current_mode = mode_num;
    const vid_mode_t* mode = VID_GetMode(mode_num);
    vid.width = mode->width;
    vid.height = mode->height;
    vid.aspect = ((float) vid.height / (float) vid.width) * (320.0f / 240.0f);
    vid.recalc_refdef = true;
    vid.numpages = 1;
    vid.colormap = host_colormap;
}

void VID_SetMode(i32 mode_num) {
    if (!VALID_MODE(mode_num)) {
        return;
    }
    if (!force_set_mode && mode_num == current_mode) {
        return;
    }
    VID_ApplyMode(mode_num);
    VID_ResizeScreen();
    Cvar_SetValue("vid_mode", (float) mode_num);
}

void VID_SetCurrentModeAsDefault(void) {
    first_update = false;
    default_mode = current_mode;
    Cvar_SetValue("_vid_default_mode_win", (float) default_mode);
}

static void VID_TestModeFor(i32 mode_num, double duration) {
    if (!VALID_MODE(mode_num)) {
        return;
    }
    previous_mode = current_mode;
    testing_mode = true;
    test_end_time = realtime + duration;
    VID_SetMode(mode_num);
}

void VID_TestMode(i32 mode_num) {
    VID_TestModeFor(mode_num, DEFAULT_TEST_TIME);
}

//==============================================================================


/*
================================================================================

CONSOLE COMMANDS

================================================================================
*/

static void VID_TestMode_f(void) {
    if (VID_IsInTestMode()) {
        return;
    }
    i32 mode_num = Q_atoi(Cmd_Argv(1));
    double test_duration = Q_atof(Cmd_Argv(2));
    if (test_duration == 0) {
        test_duration = DEFAULT_TEST_TIME;
    }
    VID_TestModeFor(mode_num, test_duration);
}

static void VID_NumModes_f(void) {
    Con_Printf("%d video modes are available\n", NUM_MODES);
}

static void VID_DescribeCurrentMode_f(void) {
    const vid_mode_t* mode = VID_GetCurrentMode();
    Con_Printf("%s\n", mode->full_description);
}

static void VID_DescribeMode_f(void) {
    i32 mode_num = Q_atoi(Cmd_Argv(1));
    const vid_mode_t* mode = VID_GetMode(mode_num);
    const char* desc = (mode ? mode->full_description : NULL);
    Con_Printf("%s\n", desc);
}

static void VID_DescribeModes_f(void) {
    for (i32 i = 0; i < NUM_MODES; i++) {
        const vid_mode_t* mode = VID_GetMode(i);
        Con_Printf("%2d: %s\n", i, mode->full_description);
    }
}

static void VID_ForceMode_f(void) {
    if (VID_IsInTestMode()) {
        return;
    }
    i32 mode_num = Q_atoi(Cmd_Argv(1));
    force_set_mode = true;
    VID_SetMode(mode_num);
    force_set_mode = false;
}

static void VID_Windowed_f(void) {
    VID_SetMode((i32) vid_windowed_mode.value);
}

static void VID_Fullscreen_f(void) {
    VID_SetMode((i32) vid_fullscreen_mode.value);
}

static void VID_Minimize_f(void) {
    // We only support minimizing windows.
    // If you're fullscreen, switch to windowed first.
    if (VID_IsWindowedMode()) {
        VID_MinimizeWindow();
    }
}

//==============================================================================


static void VID_UpdateDefaultMode(void) {
    default_mode = (i32) _vid_default_mode_win.value;
    if (!VALID_MODE(default_mode)) {
        default_mode = 0;
        Cvar_SetValue("_vid_default_mode_win", (float) default_mode);
    }
    if (!start_windowed || IS_WINDOWED(default_mode)) {
        VID_SetMode(default_mode);
    }
}

static qboolean VID_IsFirstUpdate(void) {
    if (!first_update) {
        return false;
    }
    return default_mode != (i32) _vid_default_mode_win.value;
}

void VID_UpdateModes(void) {
    if (VID_IsFirstUpdate()) {
        first_update = false;
        VID_UpdateDefaultMode();
    }
    if (testing_mode && realtime >= test_end_time) {
        testing_mode = false;
        VID_SetMode(previous_mode);
        return;
    }
    if ((i32) vid_mode.value != current_mode) {
        VID_SetMode((i32) vid_mode.value);
    }
}
