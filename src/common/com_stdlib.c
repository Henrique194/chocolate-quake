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
// com_stdlib.c -- standard library replacement functions.


#include "quakedef.h"
#include "net.h"


void* Q_malloc(size_t size) {
    return SDL_malloc(size);
}

void* Q_calloc(size_t num, size_t size) {
    return SDL_calloc(num, size);
}

void Q_free(void* ptr) {
    return SDL_free(ptr);
}

i64 Q_strtol(const char* str, char** str_end, i32 base) {
    return SDL_strtol(str, str_end, base);
}

i32 Q_atoi(const char* str) {
    i32 sign;
    if (*str == '-') {
        sign = -1;
        str++;
    } else {
        sign = 1;
    }

    i32 val = 0;

    //
    // check for hex
    //
    if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
        str += 2;
        while (true) {
            i32 c = *str++;
            if (c >= '0' && c <= '9') {
                val = (val << 4) + c - '0';
            } else if (c >= 'a' && c <= 'f') {
                val = (val << 4) + c - 'a' + 10;
            } else if (c >= 'A' && c <= 'F') {
                val = (val << 4) + c - 'A' + 10;
            } else {
                return val * sign;
            }
        }
    }

    //
    // check for character
    //
    if (str[0] == '\'') {
        return sign * str[1];
    }

    //
    // assume decimal
    //
    while (true) {
        i32 c = *str++;
        if (c < '0' || c > '9') {
            return val * sign;
        }
        val = val * 10 + c - '0';
    }
}

float Q_atof(const char* str) {
    i32 sign;
    if (*str == '-') {
        sign = -1;
        str++;
    } else {
        sign = 1;
    }

    double val = 0;

    //
    // check for hex
    //
    if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
        str += 2;
        while (true) {
            i32 c = *str++;
            if (c >= '0' && c <= '9') {
                val = (val * 16) + c - '0';
            } else if (c >= 'a' && c <= 'f') {
                val = (val * 16) + c - 'a' + 10;
            } else if (c >= 'A' && c <= 'F') {
                val = (val * 16) + c - 'A' + 10;
            } else {
                return (float) (val * sign);
            }
        }
    }

    //
    // check for character
    //
    if (str[0] == '\'') {
        return (float) (sign * str[1]);
    }

    //
    // assume decimal
    //
    i32 decimal = -1;
    i32 total = 0;
    while (true) {
        i32 c = *str++;
        if (c == '.') {
            decimal = total;
            continue;
        }
        if (c < '0' || c > '9') {
            break;
        }
        val = val * 10 + c - '0';
        total++;
    }

    if (decimal == -1) {
        return (float) (val * sign);
    }
    while (total > decimal) {
        val /= 10;
        total--;
    }

    return (float) (val * sign);
}
