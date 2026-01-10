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
// com_ext.c -- file extension functions


#include "quakedef.h"
#include "net.h"
#include <string.h>

/*
============
COM_StripExtension
============
*/
void COM_StripExtension(const char* in, char* out) {
    while (*in && *in != '.') {
        *out++ = *in++;
    }
    *out = 0;
}

/*
============
COM_FileExtension
============
*/
char* COM_FileExtension(const char* in) {
    static char exten[8];
    while (*in && *in != '.') {
        in++;
    }
    if (!*in) {
        return "";
    }
    in++;
    i32 i;
    for (i = 0; i < 7 && *in; i++, in++) {
        exten[i] = *in;
    }
    exten[i] = 0;
    return exten;
}

/*
============
COM_FileBase
============
*/
void COM_FileBase(const char* in, char* out, size_t outsize) {
    const char* s = in;
    const char* slash = in;
    const char* dot = NULL;
    while (*s) {
        if (*s == '/') {
            slash = s + 1;
        }
        if (*s == '.') {
            dot = s;
        }
        s++;
    }
    if (dot == NULL) {
        dot = s;
    }
    if (dot - slash < 2) {
        Q_strncpy(out, "?model?", outsize);
        return;
    }
    size_t len = dot - slash;
    if (len >= outsize) {
        len = outsize - 1;
    }
    Q_memcpy(out, slash, len);
    out[len] = '\0';
}


/*
==================
COM_DefaultExtension
==================
*/
void COM_DefaultExtension(char* path, const char* extension) {
    //
    // if path doesn't have a .EXT, append extension
    // (extension should include the .)
    //
    const char* src = path + Q_strlen(path) - 1;
    while (*src != '/' && src != path) {
        if (*src == '.') {
            // it has an extension
            return;
        }
        src--;
    }
    Q_strcat(path, extension);
}
