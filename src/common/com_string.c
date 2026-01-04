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


void Q_memset(void* dest, int fill, int count) {
    if ((((intptr_t) dest | count) & 3) != 0) {
        for (int i = 0; i < count; i++) {
            ((byte*) dest)[i] = (byte) fill;
        }
        return;
    }
    count >>= 2;
    fill = fill | (fill << 8) | (fill << 16) | (fill << 24);
    for (int i = 0; i < count; i++) {
        ((int*) dest)[i] = fill;
    }
}

void Q_memcpy(void* dest, void* src, int count) {
    if ((((intptr_t) dest | (intptr_t) src | count) & 3) != 0) {
        for (int i = 0; i < count; i++) {
            ((byte*) dest)[i] = ((byte*) src)[i];
        }
        return;
    }
    count >>= 2;
    for (int i = 0; i < count; i++) {
        ((int*) dest)[i] = ((int*) src)[i];
    }
}

int Q_memcmp(void* m1, void* m2, int count) {
    while (count) {
        count--;
        if (((byte*) m1)[count] != ((byte*) m2)[count]) {
            return -1;
        }
    }
    return 0;
}

void Q_strcpy(char* dest, const char* src) {
    while (*src) {
        *dest++ = *src++;
    }
    *dest = 0;
}

void Q_strncpy(char* dest, const char* src, int count) {
    while (*src && count--) {
        *dest++ = *src++;
    }
    if (count) {
        *dest = 0;
    }
}

int Q_strlen(const char* str) {
    int count = 0;
    while (str[count]) {
        count++;
    }
    return count;
}

char* Q_strrchr(char* s, char c) {
    int len = Q_strlen(s);
    s += len;
    while (len--) {
        if (*--s == c) {
            return s;
        }
    }
    return 0;
}

void Q_strcat(char* dest, const char* src) {
    dest += Q_strlen(dest);
    Q_strcpy(dest, src);
}

int Q_strcmp(const char* s1, const char* s2) {
    while (true) {
        if (*s1 != *s2) {
            // strings not equal
            return -1;
        }
        if (!*s1) {
            // strings are equal
            return 0;
        }
        s1++;
        s2++;
    }
}

int Q_strncmp(const char* s1, const char* s2, int count) {
    while (true) {
        if (!count--) {
            return 0;
        }
        if (*s1 != *s2) {
            // strings not equal
            return -1;
        }
        if (!*s1) {
            // strings are equal
            return 0;
        }
        s1++;
        s2++;
    }
}

int Q_strncasecmp(const char* s1, const char* s2, int n) {
    while (true) {
        int c1 = (int) *s1++;
        int c2 = (int) *s2++;
        if (!n--) {
            // strings are equal until end point
            return 0;
        }
        if (c1 != c2) {
            if (c1 >= 'a' && c1 <= 'z') {
                c1 -= ('a' - 'A');
            }
            if (c2 >= 'a' && c2 <= 'z') {
                c2 -= ('a' - 'A');
            }
            if (c1 != c2) {
                // strings not equal
                return -1;
            }
        }
        if (!c1) {
            // strings are equal
            return 0;
        }
    }
}

int Q_strcasecmp(const char* s1, const char* s2) {
    return Q_strncasecmp(s1, s2, 99999);
}

int Q_atoi(const char* str) {
    int sign;
    if (*str == '-') {
        sign = -1;
        str++;
    } else {
        sign = 1;
    }

    int val = 0;

    //
    // check for hex
    //
    if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
        str += 2;
        while (true) {
            int c = (int) *str++;
            if (c >= '0' && c <= '9') {
                val = (val << 4) + c - '0';
                continue;
            }
            if (c >= 'a' && c <= 'f') {
                val = (val << 4) + c - 'a' + 10;
                continue;
            }
            if (c >= 'A' && c <= 'F') {
                val = (val << 4) + c - 'A' + 10;
                continue;
            }
            return val * sign;
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
        int c = (int) *str++;
        if (c < '0' || c > '9') {
            return val * sign;
        }
        val = val * 10 + c - '0';
    }
}

float Q_atof(const char* str) {
    int sign;
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
            int c = (int) *str++;
            if (c >= '0' && c <= '9') {
                val = (val * 16) + c - '0';
                continue;
            }
            if (c >= 'a' && c <= 'f') {
                val = (val * 16) + c - 'a' + 10;
                continue;
            }
            if (c >= 'A' && c <= 'F') {
                val = (val * 16) + c - 'A' + 10;
                continue;
            }
            return (float) (val * sign);
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
    int decimal = -1;
    int total = 0;
    while (true) {
        int c = (int) *str++;
        if (c == '.') {
            decimal = total;
            continue;
        }
        if (c < '0' || c > '9')
            break;
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
