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
// com_stdio.c -- stdio library replacement functions.
//
// The following stdio replacements are necessary if one is to
// perform non-sequential reads on files reopened on pak files
// because we need the bookkeeping about file start/end positions.
// Allocating and filling in the fshandle_t structure is the users'
// responsibility when the file is initially opened.
//


#include "quakedef.h"
#include "cvar.h"
#include "net.h"
#include <errno.h>


size_t Q_fread(void* ptr, size_t size, size_t nmemb, fshandle_t* fh) {
    if (!fh) {
        errno = EBADF;
        return 0;
    }
    if (!ptr) {
        errno = EFAULT;
        return 0;
    }
    if (!size || !nmemb) { /* no error, just zero bytes wanted */
        errno = 0;
        return 0;
    }

    long byte_size = (long) (nmemb * size);
    if (byte_size > fh->length - fh->pos) {
        // just read to end.
        byte_size = fh->length - fh->pos;
    }
    long bytes_read = (long) fread(ptr, 1, byte_size, fh->file);
    fh->pos += bytes_read;

    // fread() must return the number of elements read,
    // not the total number of bytes.
    size_t nmemb_read = bytes_read / size;
    // even if the last member is only read partially
    // it is counted as a whole in the return value.
    if (bytes_read % size) {
        nmemb_read++;
    }
    return nmemb_read;
}

int Q_fseek(fshandle_t* fh, long offset, int whence) {
    // I don't care about 64 bit off_t or fseeko() here.
    // The quake file system is 32 bits, anyway.
    if (!fh) {
        errno = EBADF;
        return -1;
    }
    // the relative file position shouldn't be smaller
    // than zero or bigger than the filesize.
    switch (whence) {
        case SEEK_SET:
            break;
        case SEEK_CUR:
            offset += fh->pos;
            break;
        case SEEK_END:
            offset = fh->length + offset;
            break;
        default:
            errno = EINVAL;
            return -1;
    }
    if (offset < 0) {
        errno = EINVAL;
        return -1;
    }
    if (offset > fh->length) {
        // just seek to end
        offset = fh->length;
    }
    int ret = fseek(fh->file, fh->start + offset, SEEK_SET);
    if (ret < 0) {
        return ret;
    }
    fh->pos = offset;
    return 0;
}

long Q_ftell(const fshandle_t* fh) {
    if (!fh) {
        errno = EBADF;
        return -1;
    }
    return fh->pos;
}

void Q_rewind(fshandle_t* fh) {
    if (!fh) {
        return;
    }
    clearerr(fh->file);
    fseek(fh->file, fh->start, SEEK_SET);
    fh->pos = 0;
}

long Q_filelength(const fshandle_t* fh) {
    if (!fh) {
        errno = EBADF;
        return -1;
    }
    return fh->length;
}

int Q_feof(const fshandle_t* fh) {
    if (!fh) {
        errno = EBADF;
        return -1;
    }
    if (fh->pos >= fh->length) {
        return -1;
    }
    return 0;
}

int Q_ferror(fshandle_t* fh) {
    if (!fh) {
        errno = EBADF;
        return -1;
    }
    return ferror(fh->file);
}
