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
// com_string.c -- string library replacement functions.


#include "quakedef.h"
#include "net.h"


void* Q_memmove(void* dest, const void* src, size_t count) {
    return SDL_memmove(dest, src, count);
}

void Q_memset(void* dest, i32 fill, size_t count) {
    SDL_memset(dest, fill, count);
}

void Q_memcpy(void* dest, void* src, size_t count) {
    SDL_memcpy(dest, src, count);
}

i32 Q_memcmp(void* m1, void* m2, size_t count) {
    return SDL_memcmp(m1, m2, count);
}

void Q_strcpy(char* dest, const char* src) {
#ifdef HAVE_STRCPY
    strcpy(dest, src);
#else
    while (*src) {
        *dest++ = *src++;
    }
    *dest = 0;
#endif

}

void Q_strncpy(char* dest, const char* src, size_t count) {
#ifdef HAVE_STRNCPY
    strncpy(dest, src, count);
#else
    while (count-- && *src) {
        *dest++ = *src++;
    }
    if (count) {
        *dest = 0;
    }
#endif
}

size_t Q_strlen(const char* str) {
    return SDL_strlen(str);
}

char* Q_strrchr(const char* s, char c) {
    return SDL_strrchr(s, c);
}

void Q_strcat(char* dest, const char* src) {
#ifdef HAVE_STRCAT
    strcat(dest, src);
#else
    dest += Q_strlen(dest);
    Q_strcpy(dest, src);
#endif

}

i32 Q_strcmp(const char* s1, const char* s2) {
    return SDL_strcmp(s1, s2);
}

i32 Q_strncmp(const char* s1, const char* s2, size_t count) {
    return SDL_strncmp(s1, s2, count);
}

i32 Q_strncasecmp(const char* s1, const char* s2, size_t n) {
    return SDL_strncasecmp(s1, s2, n);
}

i32 Q_strcasecmp(const char* s1, const char* s2) {
    return Q_strncasecmp(s1, s2, 99999);
}

char* Q_strchr(const char* str, i32 c) {
    return SDL_strchr(str, c);
}

char* Q_strstr(const char* str, const char* substr) {
    return SDL_strstr(str, substr);
}
