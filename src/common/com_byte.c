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
// com_byte.c -- byte order functions

#include "quakedef.h"

qboolean bigendien;

short (*BigShort)(short l);
short (*LittleShort)(short l);
int (*BigLong)(int l);
int (*LittleLong)(int l);
float (*BigFloat)(float l);
float (*LittleFloat)(float l);

static short ShortSwap(short l) {
    byte b1 = l & 255;
    byte b2 = (l >> 8) & 255;
    return (short) ((b1 << 8) + b2);
}

static short ShortNoSwap(short l) {
    return l;
}

static int LongSwap(int l) {
    byte b1 = l & 255;
    byte b2 = (l >> 8) & 255;
    byte b3 = (l >> 16) & 255;
    byte b4 = (l >> 24) & 255;
    return ((int) b1 << 24) + ((int) b2 << 16) + ((int) b3 << 8) + b4;
}

static int LongNoSwap(int l) {
    return l;
}

static float FloatSwap(float f) {
    float res;
    const byte* b1 = (byte*) &f;
    byte* b2 = (byte*) &res;
    b2[0] = b1[3];
    b2[1] = b1[2];
    b2[2] = b1[1];
    b2[3] = b1[0];
    return res;
}

static float FloatNoSwap(float f) {
    return f;
}

//
// Set the byte swapping variables in a portable manner.
//
void COM_InitByteSwap(void) {
    byte swaptest[2] = {1, 0};

    if (*(short*) swaptest == 1) {
        bigendien = false;
        BigShort = ShortSwap;
        LittleShort = ShortNoSwap;
        BigLong = LongSwap;
        LittleLong = LongNoSwap;
        BigFloat = FloatSwap;
        LittleFloat = FloatNoSwap;
    } else {
        bigendien = true;
        BigShort = ShortNoSwap;
        LittleShort = ShortSwap;
        BigLong = LongNoSwap;
        LittleLong = LongSwap;
        BigFloat = FloatNoSwap;
        LittleFloat = FloatSwap;
    }
}

