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
// sys.h -- non-portable functions

#ifndef __SYS__
#define __SYS__

#include "quakedef.h"
#include "host.h"


// Suppresses resolution and cache size console output.
// An fullscreen DIB focus gain/loss.
extern qboolean msg_suppress_1;

extern qboolean isDedicated;


//
// file IO
//

// returns the file size
// return -1 if file is not present
// the file should be in BINARY mode for stupid OSs that care
i32 Sys_FileOpenRead(char* path, i32* hndl);

i32 Sys_FileOpenWrite(char* path);

void Sys_FileClose(i32 handle);

void Sys_FileSeek(i32 handle, i32 position);

i32 Sys_FileRead(i32 handle, void* dest, i32 count);

i32 Sys_FileWrite(i32 handle, void* data, i32 count);

i32 Sys_FileTime(char* path);

void Sys_mkdir(char* path);

//
// an error will cause the entire program to exit
//
void Sys_Error(char* error, ...);

//
// send text to the console
//
void Sys_Printf(char* fmt, ...);

void Sys_Quit(void);

double Sys_FloatTime();

char* Sys_ConsoleInput(void);

//
// Perform Key_Event () callbacks until the input que is empty
//
void Sys_SendKeyEvents();

void Sys_LowFPPrecision(void);

void Sys_HighFPPrecision(void);

quakeparms_t* Sys_Init(i32 argc, char* argv[]);

#endif