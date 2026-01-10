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
#include "console.h"
#include "sys.h"
#include "zone.h"


void SZ_Alloc(sizebuf_t* buf, i32 startsize) {
    if (startsize < 256) {
        startsize = 256;
    }
    buf->data = Hunk_AllocName(startsize, "sizebuf");
    buf->maxsize = startsize;
    buf->cursize = 0;
}

void SZ_Free(sizebuf_t* buf) {
    buf->cursize = 0;
}

void SZ_Clear(sizebuf_t* buf) {
    buf->cursize = 0;
}

void* SZ_GetSpace(sizebuf_t* buf, i32 length) {
    if (buf->cursize + length > buf->maxsize) {
        if (!buf->allowoverflow) {
            Sys_Error("SZ_GetSpace: overflow without allowoverflow set");
        }
        if (length > buf->maxsize) {
            Sys_Error("SZ_GetSpace: %i is > full buffer size", length);
        }
        buf->overflowed = true;
        Con_Printf("SZ_GetSpace: overflow");
        SZ_Clear(buf);
    }
    void* data = buf->data + buf->cursize;
    buf->cursize += length;
    return data;
}

void SZ_Write(sizebuf_t* buf, void* data, i32 length) {
    Q_memcpy(SZ_GetSpace(buf, length), data, length);
}

void SZ_Print(sizebuf_t* buf, char* data) {
    i32 len = (i32) Q_strlen(data) + 1;
    void* dest;
    if (buf->data[buf->cursize - 1]) {
        // no trailing 0
        dest = SZ_GetSpace(buf, len);
    } else {
        // write over trailing 0
        dest = SZ_GetSpace(buf, len - 1) - 1;
    }
    Q_memcpy(dest, data, len);
}
