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
// vid_menu.c -- handles video menu drawing and user input.


#include "quakedef.h"
#include "vid_menu.h"
#include "vid_modes.h"


#define VID_ROW_SIZE     3
#define MAX_COLUMN_SIZE  5
#define MODE_AREA_HEIGHT (MAX_COLUMN_SIZE + 6)


/*
================================================================================

MENU DRAWING

================================================================================
*/

void M_Menu_Options_f(void);
void M_DrawPic(int x, int y, qpic_t* pic);
void M_Print(int cx, int cy, char* str);
void M_PrintWhite(int cx, int cy, char* str);
void M_DrawCharacter(int cx, int line, int num);

static int vid_line;


static void M_DrawCursor() {
    int x = 8 + (vid_line % VID_ROW_SIZE) * 13 * 8;
    int y = 36 + 2 * 8 + (vid_line / VID_ROW_SIZE) * 8;
    if (vid_line >= 3) {
        y += 3 * 8;
    }
    // Blink cursor every 0.25 sec.
    int cursor = 12 + ((int) (realtime * 4) & 1);
    M_DrawCharacter(x, y, cursor);
}

static void VID_MenuDrawFooter(void) {
    static char temp[64];

    int x = (9 * 8);
    int y = 36 + (MODE_AREA_HEIGHT * 8) + 8;
    M_Print(x, y, "Press Enter to set mode");

    x -= (3 * 8);
    y += (2 * 8);
    sprintf(temp, "T to test mode for %d seconds", DEFAULT_TEST_TIME);
    M_Print(x, y, temp);

    const vid_mode_t* mode = VID_GetCurrentMode();
    sprintf(temp, "D to set default: %s", mode->full_description);
    x -= (4 * 8);
    y += (2 * 8);
    M_Print(x, y, temp);

    mode = VID_GetDefaultMode();
    sprintf(temp, "Current default: %s", mode->full_description);
    x += 8;
    y += 8;
    M_Print(x, y, temp);

    x += (12 * 8);
    y += (2 * 8);
    M_Print(x, y, "Esc to exit");
}

static void VID_MenuDrawTestingMode(void) {
    static char temp[64];

    const vid_mode_t* mode = VID_GetMode(vid_line);
    sprintf(temp, "TESTING %s", mode->description);
    int x = (13 * 8);
    int y = 36 + (MODE_AREA_HEIGHT * 8) + (8 * 4);
    M_Print(x, y, temp);

    x -= (4 * 8);
    y += (2 * 8);
    sprintf(temp, "Please wait %d seconds...", DEFAULT_TEST_TIME);
    M_Print(x, y, temp);
}

static void VID_MenuDrawFullScreenModes(void) {
    M_Print(12 * 8, 36 + (4 * 8), "Fullscreen Modes");

    int column = 16;
    int row = 36 + 6 * 8;

    const vid_mode_t* cur_mode = VID_GetCurrentMode();

    for (int i = NUM_WINDOWED_MODES; i < NUM_MODES; i++) {
        const vid_mode_t* mode = VID_GetMode(i);
        if (mode == cur_mode) {
            M_PrintWhite(column, row, mode->description);
        } else {
            M_Print(column, row, mode->description);
        }
        column += (13 * 8);
        if (((i - 3) % VID_ROW_SIZE) == (VID_ROW_SIZE - 1)) {
            column = 16;
            row += 8;
        }
    }
}

static void VID_MenuDrawWindowedModes(void) {
    M_Print(13 * 8, 36, "Windowed Modes");

    int column = 16;
    int row = 36 + (2 * 8);
    const vid_mode_t* cur_mode = VID_GetCurrentMode();

    for (int i = 0; i < NUM_WINDOWED_MODES; i++) {
        const vid_mode_t* mode = VID_GetMode(i);
        if (mode == cur_mode) {
            M_PrintWhite(column, row, mode->description);
        } else {
            M_Print(column, row, mode->description);
        }
        column += (13 * 8);
    }
}

static void VID_MenuDrawTitle(void) {
    qpic_t* p = Draw_CachePic("gfx/vidmodes.lmp");
    int x = (320 - p->width) / 2;
    int y = 4;
    M_DrawPic(x, y, p);
}

void VID_MenuDraw(void) {
    VID_MenuDrawTitle();
    VID_MenuDrawWindowedModes();
    VID_MenuDrawFullScreenModes();
    if (VID_IsInTestMode()) {
        VID_MenuDrawTestingMode();
    } else {
        VID_MenuDrawFooter();
        M_DrawCursor();
    }
}

//==============================================================================


/*
================================================================================

MENU KEY HANDLING

================================================================================
*/

static void VID_DoMenuSound(void) {
    S_LocalSound("misc/menu1.wav");
}

static void VID_MenuSelectMode(void) {
    VID_SetMode(vid_line);
}

static void VID_MenuDownArrow(void) {
    vid_line += VID_ROW_SIZE;
    if (vid_line >= NUM_MODES) {
        vid_line -=
            ((NUM_MODES + (VID_ROW_SIZE - 1)) / VID_ROW_SIZE) * VID_ROW_SIZE;
        while (vid_line < 0) {
            vid_line += VID_ROW_SIZE;
        }
    }
}

static void VID_MenuUpArrow(void) {
    vid_line -= VID_ROW_SIZE;
    if (vid_line < 0) {
        vid_line +=
            ((NUM_MODES + (VID_ROW_SIZE - 1)) / VID_ROW_SIZE) * VID_ROW_SIZE;
        while (vid_line >= NUM_MODES) {
            vid_line -= VID_ROW_SIZE;
        }
    }
}

static void VID_MenuRightArrow(void) {
    vid_line = ((vid_line / VID_ROW_SIZE) * VID_ROW_SIZE) +
               ((vid_line + 4) % VID_ROW_SIZE);
    if (vid_line >= NUM_MODES) {
        vid_line = (vid_line / VID_ROW_SIZE) * VID_ROW_SIZE;
    }
}

static void VID_MenuLeftArrow(void) {
    vid_line = ((vid_line / VID_ROW_SIZE) * VID_ROW_SIZE) +
               ((vid_line + 2) % VID_ROW_SIZE);
    if (vid_line >= NUM_MODES) {
        vid_line = NUM_MODES - 1;
    }
}

static void VID_MenuEscape(void) {
    M_Menu_Options_f();
}

void VID_MenuKey(int key) {
    if (VID_IsInTestMode()) {
        return;
    }
    switch (key) {
        case K_ESCAPE:
            VID_DoMenuSound();
            VID_MenuEscape();
            break;
        case K_LEFTARROW:
            VID_DoMenuSound();
            VID_MenuLeftArrow();
            break;
        case K_RIGHTARROW:
            VID_DoMenuSound();
            VID_MenuRightArrow();
            break;
        case K_UPARROW:
            VID_DoMenuSound();
            VID_MenuUpArrow();
            break;
        case K_DOWNARROW:
            VID_DoMenuSound();
            VID_MenuDownArrow();
            break;
        case K_ENTER:
            VID_DoMenuSound();
            VID_MenuSelectMode();
            break;
        case 'T':
        case 't':
            VID_DoMenuSound();
            VID_TestMode(vid_line);
            break;
        case 'D':
        case 'd':
            VID_DoMenuSound();
            VID_SetCurrentModeAsDefault();
            break;
        default:
            break;
    }
}

//==============================================================================
