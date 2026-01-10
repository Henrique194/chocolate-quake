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


#include "quakedef.h"
#include "cmd.h"
#include "console.h"
#include "cvar.h"
#include "sys.h"


void COM_InitByteSwap(void);

i32 static_registered = 1;


cvar_t registered = {"registered", "0"};
static cvar_t cmdline = {"cmdline", "0", false, true};

// This graphic needs to be in the pak file to use registered features.
static u16 pop[] = {
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x6600, 0x0000, 0x0000, 0x0000, 0x6600, 0x0000, 0x0000, 0x0066,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0067, 0x0000, 0x0000, 0x6665, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0065, 0x6600, 0x0063, 0x6561, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0061, 0x6563, 0x0064, 0x6561, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0061, 0x6564, 0x0064, 0x6564, 0x0000, 0x6469, 0x6969, 0x6400,
    0x0064, 0x6564, 0x0063, 0x6568, 0x6200, 0x0064, 0x6864, 0x0000, 0x6268,
    0x6563, 0x0000, 0x6567, 0x6963, 0x0064, 0x6764, 0x0063, 0x6967, 0x6500,
    0x0000, 0x6266, 0x6769, 0x6a68, 0x6768, 0x6a69, 0x6766, 0x6200, 0x0000,
    0x0062, 0x6566, 0x6666, 0x6666, 0x6666, 0x6562, 0x0000, 0x0000, 0x0000,
    0x0062, 0x6364, 0x6664, 0x6362, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0062, 0x6662, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0061,
    0x6661, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x6500,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x6400, 0x0000,
    0x0000, 0x0000
};


/*
================
COM_CheckRegistered

Looks for the pop.txt file and verifies it.
Sets the "registered" cvar.
Immediately exits out if an alternate game was attempted to be started without
being registered.
================
*/
static void COM_CheckRegistered(void) {
    i32 h;
    COM_OpenFile("gfx/pop.lmp", &h);
    static_registered = 0;
    if (h == -1) {
        Con_Printf("Playing shareware version.\n");
        if (com_modified) {
            Sys_Error("You must have the registered version to use modified games");
        }
        return;
    }

    u16 check[128];
    Sys_FileRead(h, check, sizeof(check));
    COM_CloseFile(h);
    for (i32 i = 0; i < 128; i++) {
        if (pop[i] != (u16) BigShort(check[i])) {
            Sys_Error("Corrupted data file.");
        }
    }

    Cvar_Set("cmdline", com_cmdline);
    Cvar_Set("registered", "1");
    static_registered = 1;
    Con_Printf("Playing registered version.\n");
}


/*
================
COM_Init
================
*/
void COM_Init(char* basedir) {
    COM_InitByteSwap();

    Cvar_RegisterVariable(&registered);
    Cvar_RegisterVariable(&cmdline);
    Cmd_AddCommand("path", COM_Path_f);

    COM_InitFilesystem();
    COM_CheckRegistered();
}
