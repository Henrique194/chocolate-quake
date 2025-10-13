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


#include "snd_codec.h"
#include "console.h"
#include "snd_mp3.h"
#include "snd_vorbis.h"
#include "snd_flac.h"
#include "snd_wave.h"
#include "zone.h"
#include <stdio.h>


static snd_codec_t* codecs;


/*
=================
S_CodecRegister
=================
*/
void S_CodecRegister(snd_codec_t* codec) {
    codec->next = codecs;
    codecs = codec;
}

/*
=================
S_CodecInit
=================
*/
void S_CodecInit() {
    snd_codec_t* codec;
    codecs = NULL;

    S_CodecRegister(&mp3_codec);
    S_CodecRegister(&vorbis_codec);
    S_CodecRegister(&flac_codec);
    S_CodecRegister(&wav_codec);

    codec = codecs;
    while (codec) {
        codec->initialize();
        codec = codec->next;
    }
}

/*
=================
S_CodecShutdown
=================
*/
void S_CodecShutdown() {
    snd_codec_t* codec = codecs;
    while (codec) {
        codec->shutdown();
        codec = codec->next;
    }
    codecs = NULL;
}

/*
=================
S_CodecOpenStream
=================
*/
snd_stream_t* S_CodecOpenStreamType(const char* filename, unsigned int type,
                                    qboolean loop) {
    if (type == CODECTYPE_NONE) {
        Con_Printf("Bad type for %s\n", filename);
        return NULL;
    }

    snd_codec_t* codec = codecs;
    while (codec) {
        if (type == codec->type) {
            break;
        }
        codec = codec->next;
    }
    if (!codec) {
        Con_Printf("Unknown type for %s\n", filename);
        return NULL;
    }
    snd_stream_t* stream = S_CodecUtilOpen(filename, codec, loop);
    if (stream) {
        if (codec->codec_open(stream)) {
            stream->status = STREAM_PLAY;
        } else {
            S_CodecUtilClose(&stream);
        }
    }
    return stream;
}

void S_CodecCloseStream(snd_stream_t* stream) {
    stream->status = STREAM_NONE;
    stream->codec->codec_close(stream);
}

int S_CodecRewindStream(snd_stream_t* stream) {
    return stream->codec->codec_rewind(stream);
}

int S_CodecJumpToOrder(snd_stream_t* stream, int to) {
    if (stream->codec->codec_jump) {
        return stream->codec->codec_jump(stream, to);
    }
    return -1;
}

int S_CodecReadStream(snd_stream_t* stream, int bytes, void* buffer) {
    return stream->codec->codec_read(stream, bytes, buffer);
}

/* Util functions (used by codecs) */

snd_stream_t* S_CodecUtilOpen(const char* filename, snd_codec_t* codec,
                              qboolean loop) {
    snd_stream_t* stream;
    FILE* handle;
    long length;

    /* Try to open the file */
    length = (long) COM_FindMusicTrack(filename, &handle);
    if (length == -1) {
        Con_DPrintf("Couldn't open %s\n", filename);
        return NULL;
    }

    /* Allocate a stream, Z_Malloc zeroes its content */
    stream = (snd_stream_t*) Z_Malloc(sizeof(snd_stream_t));
    stream->codec = codec;
    stream->loop = loop;
    stream->fh.file = handle;
    stream->fh.start = ftell(handle);
    stream->fh.pos = 0;
    stream->fh.length = length;
    Q_strncpy(stream->name, filename, MAX_QPATH);

    return stream;
}

void S_CodecUtilClose(snd_stream_t** stream) {
    fclose((*stream)->fh.file);
    Z_Free(*stream);
    *stream = NULL;
}

int S_CodecIsAvailable(unsigned int type) {
    snd_codec_t* codec = codecs;
    while (codec) {
        if (type == codec->type) {
            return codec->initialized;
        }
        codec = codec->next;
    }
    return -1;
}
