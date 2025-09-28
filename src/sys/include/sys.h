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
// sys.h -- non-portable functions

#ifndef __SYS__
#define __SYS__

//
// file IO
//

// returns the file size
// return -1 if file is not present
// the file should be in BINARY mode for stupid OSs that care
int Sys_FileOpenRead(char* path, int* hndl);

int Sys_FileOpenWrite(char* path);

void Sys_FileClose(int handle);

void Sys_FileSeek(int handle, int position);

int Sys_FileRead(int handle, void* dest, int count);

int Sys_FileWrite(int handle, void* data, int count);

int Sys_FileTime(char* path);

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

quakeparms_t* Sys_Init(int argc, char* argv[]);

#endif
