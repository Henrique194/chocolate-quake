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
#include "net.h"

char com_token[1024];


/*
==============
COM_Parse

Parse a token out of a string
==============
*/
char* COM_Parse(char* data) {
    i32 c;

    i32 len = 0;
    com_token[0] = 0;

    if (!data) {
        return NULL;
    }

// skip whitespace
skipwhite:
    // Skip control chars and space.
    while ((c = *data) <= ' ') {
        if (c == 0) {
            // End of file.
            return NULL;
        }
        data++;
    }

    // Skip // comments.
    if (c == '/' && data[1] == '/') {
        while (*data && *data != '\n') {
            data++;
        }
        goto skipwhite;
    }


    // Handle quoted strings specially.
    if (c == '\"') {
        data++;
        while (true) {
            c = *data++;
            if (c == '\"' || !c) {
                com_token[len] = 0;
                return data;
            }
            com_token[len] = c;
            len++;
        }
    }

    // Parse single characters.
    if (c == '{' || c == '}' || c == ')' || c == '(' || c == '\'' || c == ':') {
        com_token[len] = c;
        len++;
        com_token[len] = 0;
        return data + 1;
    }

    // Parse a regular word.
    do {
        com_token[len] = c;
        data++;
        len++;
        c = *data;
        if (c == '{' || c == '}' || c == ')' || c == '(' || c == '\'' || c == ':') {
            break;
        }
    } while (c > ' ');

    com_token[len] = 0;
    return data;
}
