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
// com_msg.c -- message IO functions.
// Handles byte ordering and avoids alignment errors.


#include "quakedef.h"
#include "net.h"
#include "sys.h"


i32 msg_readcount;
qboolean msg_badread;


void MSG_WriteChar(sizebuf_t* sb, i32 c) {
#ifdef PARANOID
    if (c < -128 || c > 127) {
        Sys_Error("MSG_WriteChar: range error");
    }
#endif
    byte* buf = SZ_GetSpace(sb, 1);
    buf[0] = (byte) c;
}

void MSG_WriteByte(sizebuf_t* sb, i32 c) {
#ifdef PARANOID
    if (c < 0 || c > 255) {
        Sys_Error("MSG_WriteByte: range error");
    }
#endif
    byte* buf = SZ_GetSpace(sb, 1);
    buf[0] = (byte) c;
}

void MSG_WriteShort(sizebuf_t* sb, i32 c) {
#ifdef PARANOID
    if (c < ((i16) 0x8000) || c > (i16) 0x7fff) {
        Sys_Error("MSG_WriteShort: range error");
    }
#endif
    byte* buf = SZ_GetSpace(sb, 2);
    buf[0] = c & 0xff;
    buf[1] = (byte) (c >> 8);
}

void MSG_WriteLong(sizebuf_t* sb, i32 c) {
    byte* buf = SZ_GetSpace(sb, 4);
    buf[0] = c & 0xff;
    buf[1] = (c >> 8) & 0xff;
    buf[2] = (c >> 16) & 0xff;
    buf[3] = c >> 24;
}

void MSG_WriteFloat(sizebuf_t* sb, float f) {
    i32* l = (i32*) &f;
    *l = LittleLong(*l);
    SZ_Write(sb, l, 4);
}

void MSG_WriteString(sizebuf_t* sb, char* s) {
    if (!s) {
        SZ_Write(sb, "", 1);
    } else {
        SZ_Write(sb, s, (i32) Q_strlen(s) + 1);
    }
}

void MSG_WriteCoord(sizebuf_t* sb, float f) {
    MSG_WriteShort(sb, (i32) (f * 8));
}

void MSG_WriteAngle(sizebuf_t* sb, float f) {
    MSG_WriteByte(sb, ((i32) f * 256 / 360) & 255);
}


void MSG_BeginReading(void) {
    msg_readcount = 0;
    msg_badread = false;
}

// returns -1 and sets msg_badread if no more characters are available
i32 MSG_ReadChar(void) {
    if (msg_readcount + 1 > net_message.cursize) {
        msg_badread = true;
        return -1;
    }
    char c = (char) net_message.data[msg_readcount];
    msg_readcount++;
    return c;
}

i32 MSG_ReadByte(void) {
    if (msg_readcount + 1 > net_message.cursize) {
        msg_badread = true;
        return -1;
    }
    byte c = net_message.data[msg_readcount];
    msg_readcount++;
    return c;
}

i32 MSG_ReadShort(void) {
    if (msg_readcount + 2 > net_message.cursize) {
        msg_badread = true;
        return -1;
    }
    const byte* data = &net_message.data[msg_readcount];
    i16 c = (i16) (data[0] + (data[1] << 8));
    msg_readcount += 2;
    return c;
}

i32 MSG_ReadLong(void) {
    if (msg_readcount + 4 > net_message.cursize) {
        msg_badread = true;
        return -1;
    }
    const byte* data = &net_message.data[msg_readcount];
    i32 c = data[0] + (data[1] << 8) + (data[2] << 16) + (data[3] << 24);
    msg_readcount += 4;
    return c;
}

float MSG_ReadFloat(void) {
    float f;
    byte* b = (byte*) &f;
    const byte* data = &net_message.data[msg_readcount];
    b[0] = data[0];
    b[1] = data[1];
    b[2] = data[2];
    b[3] = data[3];
    msg_readcount += 4;
    i32* l = (i32*) b;
    *l = LittleLong(*l);
    return f;
}

char* MSG_ReadString(void) {
    static char string[2048];

    i32 l = 0;
    do {
        i32 c = MSG_ReadChar();
        if (c == -1 || c == 0) {
            break;
        }
        string[l] = (char) c;
        l++;
    } while (l < sizeof(string) - 1);

    string[l] = 0;

    return string;
}

float MSG_ReadCoord(void) {
    return (float) MSG_ReadShort() * (1.0f / 8);
}

float MSG_ReadAngle(void) {
    return (float) MSG_ReadChar() * (360.0f / 256);
}
