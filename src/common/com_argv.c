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

#define NUM_SAFE_ARGVS 7

static char* largv[MAX_NUM_ARGVS + NUM_SAFE_ARGVS + 1];
static char* argvdummy = " ";

char com_cmdline[CMDLINE_LENGTH];

static char* safeargvs[NUM_SAFE_ARGVS] = {
    "-stdvid",
    "-nolan",
    "-nosound",
    "-nocdaudio",
    "-nojoy",
    "-nomouse",
    "-dibonly"
};

int com_argc;
char** com_argv;


/*
================
COM_CheckParm

Returns the position (1 to argc-1) in the program's argument list
where the given parameter apears, or 0 if not present
================
*/
int COM_CheckParm(const char* parm) {
    for (int i = 1; i < com_argc; i++) {
        if (!com_argv[i]) {
            // NEXTSTEP sometimes clears appkit vars.
            continue;
        }
        if (!Q_strcmp(parm, com_argv[i])) {
            return i;
        }
    }
    return 0;
}


/*
================
COM_InitArgv
================
*/
void COM_InitArgv(int argc, char** argv) {
    // Reconstitute the command line for the cmdline externally visible cvar.
    int n = 0;
    for (int j = 0; (j < MAX_NUM_ARGVS) && (j < argc); j++) {
        int i = 0;
        while ((n < (CMDLINE_LENGTH - 1)) && argv[j][i]) {
            com_cmdline[n++] = argv[j][i++];
        }
        if (n >= (CMDLINE_LENGTH - 1)) {
            break;
        }
        com_cmdline[n++] = ' ';
    }
    com_cmdline[n] = 0;

    qboolean safe = false;
    for (com_argc = 0; (com_argc < MAX_NUM_ARGVS) && (com_argc < argc); com_argc++) {
        largv[com_argc] = argv[com_argc];
        if (!Q_strcmp("-safe", argv[com_argc])) {
            safe = true;
        }
    }
    if (safe) {
        // Force all the safe-mode switches.
        // Note that we reserved extra space in case we need
        // to add these, so we don't need an overflow check.
        for (int i = 0; i < NUM_SAFE_ARGVS; i++) {
            largv[com_argc] = safeargvs[i];
            com_argc++;
        }
    }
    largv[com_argc] = argvdummy;
    com_argv = largv;

    if (COM_CheckParm("-rogue")) {
        rogue = true;
        standard_quake = false;
    }
    if (COM_CheckParm("-hipnotic")) {
        hipnotic = true;
        standard_quake = false;
    }
}
