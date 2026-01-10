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

#ifndef _SND_CODEC_H_
#define _SND_CODEC_H_


#include "quakedef.h"


typedef struct snd_info_s {
    i32 rate;
    i32 bits, width;
    i32 channels;
    i32 samples;
    i32 blocksize;
    i32 size;
    i32 dataofs;
} snd_info_t;

typedef enum {
    STREAM_NONE = -1,
    STREAM_INIT,
    STREAM_PAUSE,
    STREAM_PLAY
} stream_status_t;

typedef struct snd_codec_s snd_codec_t;

typedef struct snd_stream_s {
    fshandle_t fh;
    char name[MAX_QPATH]; /* name of the source file */
    snd_info_t info;
    stream_status_t status;
    snd_codec_t* codec; /* codec handling this stream */
    qboolean loop;
    void* priv; /* data private to the codec. */
} snd_stream_t;


void S_CodecInit(void);
void S_CodecShutdown(void);

/* Callers of the following S_CodecOpenStream* functions
 * are reponsible for attaching any path to the filename */

snd_stream_t* S_CodecOpenStreamType(const char* filename, u32 type,
                                    qboolean loop);
/* Decides according to the required type. */

snd_stream_t* S_CodecOpenStreamAny(const char* filename, qboolean loop);
/* Decides according to file extension. if the
	 * name has no extension, try all available. */

snd_stream_t* S_CodecOpenStreamExt(const char* filename, qboolean loop);
/* Decides according to file extension. the name
	 * MUST have an extension. */

void S_CodecCloseStream(snd_stream_t* stream);
i32 S_CodecReadStream(snd_stream_t* stream, i32 bytes, void* buffer);
i32 S_CodecRewindStream(snd_stream_t* stream);
i32 S_CodecJumpToOrder(snd_stream_t* stream, i32 to);

snd_stream_t* S_CodecUtilOpen(const char* filename, snd_codec_t* codec,
                              qboolean loop);
void S_CodecUtilClose(snd_stream_t** stream);


#define CODECTYPE_NONE   0
#define CODECTYPE_MID    (1U << 0)
#define CODECTYPE_MOD    (1U << 1)
#define CODECTYPE_FLAC   (1U << 2)
#define CODECTYPE_WAV    (1U << 3)
#define CODECTYPE_MP3    (1U << 4)
#define CODECTYPE_VORBIS (1U << 5)
#define CODECTYPE_OPUS   (1U << 6)
#define CODECTYPE_UMX    (1U << 7)

#define CODECTYPE_WAVE CODECTYPE_WAV
#define CODECTYPE_MIDI CODECTYPE_MID

i32 S_CodecIsAvailable(u32 type);
/* return 1 if available, 0 if codec failed init
	 * or -1 if no such codec is present. */


/* Codec internals */
typedef qboolean (*CODEC_INIT)(void);
typedef void (*CODEC_SHUTDOWN)(void);
typedef qboolean (*CODEC_OPEN)(snd_stream_t* stream);
typedef i32 (*CODEC_READ)(snd_stream_t* stream, i32 bytes, void* buffer);
typedef i32 (*CODEC_REWIND)(snd_stream_t* stream);
typedef i32 (*CODEC_JUMP)(snd_stream_t* stream, i32 order);
typedef void (*CODEC_CLOSE)(snd_stream_t* stream);

struct snd_codec_s {
    u32 type;    /* handled data type. (1U << n) */
    qboolean initialized; /* init succeedded */
    const char* ext;      /* expected extension */
    CODEC_INIT initialize;
    CODEC_SHUTDOWN shutdown;
    CODEC_OPEN codec_open;
    CODEC_READ codec_read;
    CODEC_REWIND codec_rewind;
    CODEC_JUMP codec_jump;
    CODEC_CLOSE codec_close;
    snd_codec_t* next;
};

#endif /* _SND_CODEC_H_ */
