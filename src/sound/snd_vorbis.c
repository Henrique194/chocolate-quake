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


#include "snd_vorbis.h"
#include "console.h"
#include "zone.h"
#include <SDL_endian.h>

#define OV_EXCLUDE_STATIC_CALLBACKS
#include <vorbis/vorbisfile.h>

/* Vorbis codec can return the samples in a number of different
 * formats, we use the standard signed short format. */
#define VORBIS_SAMPLEBITS  16
#define VORBIS_SAMPLEWIDTH 2
#define VORBIS_SIGNED_DATA 1

/* CALLBACK FUNCTIONS: */

static i32 ovc_fclose(void* f) {
    return 0; /* we fclose() elsewhere. */
}

static i32 ovc_fseek(void* f, ogg_int64_t off, i32 whence) {
    if (f == NULL)
        return (-1);
    return Q_fseek(f, off, whence);
}

static ov_callbacks ovc_qfs = {
    (size_t(*)(void*, size_t, size_t, void*)) Q_fread,
    (i32 (*)(void*, ogg_int64_t, int)) ovc_fseek, (i32 (*)(void*)) ovc_fclose,
    (long (*)(void*)) Q_ftell};

static qboolean S_VORBIS_CodecInitialize(void) {
    return true;
}

static void S_VORBIS_CodecShutdown(void) {
}

static qboolean S_VORBIS_CodecOpenStream(snd_stream_t* stream) {
    OggVorbis_File* ovFile;
    vorbis_info* ovf_info;
    i64 numstreams;
    i32 res;

    ovFile = (OggVorbis_File*) Z_Malloc(sizeof(OggVorbis_File));
    stream->priv = ovFile;
    res = ov_open_callbacks(&stream->fh, ovFile, NULL, 0, ovc_qfs);
    if (res != 0) {
        Con_Printf("%s is not a valid Ogg Vorbis file (error %i).\n",
                   stream->name, res);
        goto _fail;
    }

    if (!ov_seekable(ovFile)) {
        Con_Printf("Stream %s not seekable.\n", stream->name);
        goto _fail;
    }

    ovf_info = ov_info(ovFile, 0);
    if (!ovf_info) {
        Con_Printf("Unable to get stream info for %s.\n", stream->name);
        goto _fail;
    }

    /* FIXME: handle section changes */
    numstreams = ov_streams(ovFile);
    if (numstreams != 1) {
        Con_Printf("More than one (%ld) stream in %s.\n", numstreams,
                   stream->name);
        goto _fail;
    }

    if (ovf_info->channels != 1 && ovf_info->channels != 2) {
        Con_Printf("Unsupported number of channels %d in %s\n",
                   ovf_info->channels, stream->name);
        goto _fail;
    }

    stream->info.rate = ovf_info->rate;
    stream->info.channels = ovf_info->channels;
    stream->info.bits = VORBIS_SAMPLEBITS;
    stream->info.width = VORBIS_SAMPLEWIDTH;

    return true;
_fail:
    if (res == 0)
        ov_clear(ovFile);
    Z_Free(ovFile);
    return false;
}

static i32 S_VORBIS_CodecReadStream(snd_stream_t* stream, i32 bytes,
                                    void* buffer) {
    i32 section; /* FIXME: handle section changes */
    i32 cnt, res, rem;
    char* ptr;

    cnt = 0;
    rem = bytes;
    ptr = (char*) buffer;

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    qboolean host_bigendian = true;
#else
    qboolean host_bigendian = false;
#endif

    while (1) {
        /* # ov_read() from libvorbisfile returns the decoded PCM audio
	 *   in requested endianness, signedness and word size.
	 * # ov_read() from Tremor (libvorbisidec) returns decoded audio
	 *   always in host-endian, signed 16 bit PCM format.
	 * # For both of the libraries, if the audio is multichannel,
	 *   the channels are interleaved in the output buffer.
	 */
        res = ov_read((OggVorbis_File*) stream->priv, ptr, rem, host_bigendian,
                      VORBIS_SAMPLEWIDTH, VORBIS_SIGNED_DATA, &section);
        if (res <= 0)
            break;
        rem -= res;
        cnt += res;
        if (rem <= 0)
            break;
        ptr += res;
    }

    if (res < 0)
        return res;
    return cnt;
}

static void S_VORBIS_CodecCloseStream(snd_stream_t* stream) {
    ov_clear((OggVorbis_File*) stream->priv);
    Z_Free(stream->priv);
    S_CodecUtilClose(&stream);
}

static i32 S_VORBIS_CodecRewindStream(snd_stream_t* stream) {
    /*
     * For libvorbisfile, the ov_time_seek() position argument
     * is seconds as doubles, whereas for Tremor libvorbisidec
     * it is milliseconds as 64 bit integers.
     */
    return ov_time_seek((OggVorbis_File*) stream->priv, 0);
}

snd_codec_t vorbis_codec = {
    CODECTYPE_VORBIS,
    true, /* always available. */
    "ogg",
    S_VORBIS_CodecInitialize,
    S_VORBIS_CodecShutdown,
    S_VORBIS_CodecOpenStream,
    S_VORBIS_CodecReadStream,
    S_VORBIS_CodecRewindStream,
    NULL, /* jump */
    S_VORBIS_CodecCloseStream,
    NULL
};
