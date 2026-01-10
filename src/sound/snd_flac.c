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

#include "snd_flac.h"
#include "console.h"
#include "zone.h"
#include <FLAC/stream_decoder.h>
#include <stdlib.h>
#include <string.h>

typedef size_t FLAC_SIZE_T;

typedef struct {
    FLAC__StreamDecoder* decoder;
    fshandle_t* file;
    snd_info_t* info;
    byte* buffer;
    i32 size;
    i32 pos;
    i32 error;
} flacfile_t;

static void FLAC_Error_f(
    const FLAC__StreamDecoder* decoder,
    FLAC__StreamDecoderErrorStatus status,
    void* client_data
) {
    flacfile_t* ff = client_data;
    ff->error = -1;
    Con_Printf("FLAC: decoder error %i\n", status);
}

static FLAC__StreamDecoderReadStatus FLAC_Read_f(
    const FLAC__StreamDecoder* decoder,
    FLAC__byte buffer[],
    FLAC_SIZE_T* bytes,
    void* client_data
) {
    const flacfile_t* ff = client_data;
    if (*bytes <= 0) {
        return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
    }
    *bytes = Q_fread(buffer, 1, *bytes, ff->file);
    if (Q_ferror(ff->file)) {
        return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
    }
    if (*bytes == 0) {
        return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
    }
    return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
}

static FLAC__StreamDecoderSeekStatus FLAC_Seek_f(
    const FLAC__StreamDecoder* decoder,
    u64 absolute_byte_offset,
    void* client_data
) {
    const flacfile_t* ff = client_data;
    fshandle_t* fh = ff->file;
    const i64 offset = (i64) absolute_byte_offset;
    const i32 whence = SEEK_SET;
    if (Q_fseek(fh, offset, whence) < 0) {
        return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;
    }
    return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
}

static FLAC__StreamDecoderTellStatus FLAC_Tell_f(
    const FLAC__StreamDecoder* decoder,
    u64* absolute_byte_offset,
    void* client_data
) {
    const flacfile_t* ff = client_data;
    const i64 pos = Q_ftell(ff->file);
    if (pos < 0) {
        return FLAC__STREAM_DECODER_TELL_STATUS_ERROR;
    }
    *absolute_byte_offset = (u64) pos;
    return FLAC__STREAM_DECODER_TELL_STATUS_OK;
}

static FLAC__StreamDecoderLengthStatus FLAC_Length_f(
    const FLAC__StreamDecoder* decoder,
    u64* stream_length,
    void* client_data
) {
    const flacfile_t* ff = client_data;
    *stream_length = (u64) Q_filelength(ff->file);
    return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
}

static FLAC__bool FLAC_EOF_f(
    const FLAC__StreamDecoder* decoder,
    void* client_data
) {
    const flacfile_t* ff = client_data;
    return Q_feof(ff->file) != 0;
}

static FLAC__StreamDecoderWriteStatus FLAC_Write_f(
    const FLAC__StreamDecoder* decoder,
    const FLAC__Frame* frame,
    const i32* const buffer[],
    void* client_data
) {
    flacfile_t* ff = client_data;

    if (!ff->buffer) {
        ff->buffer = (byte*) Q_malloc(ff->info->blocksize * ff->info->channels
                                    * ff->info->width);
        if (!ff->buffer) {
            ff->error = -1; // needn't set this here, but
            Con_Printf("Insufficient memory for flac audio\n");
            return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
        }
    }

    if (ff->info->channels == 1) {
        u32 i;
        const i32* in = buffer[0];

        if (ff->info->bits == 8) {
            byte* out = ff->buffer;
            for (i = 0; i < frame->header.blocksize; i++)
                *out++ = *in++ + 128;
        } else {
            i16* out = (i16*) ff->buffer;
            for (i = 0; i < frame->header.blocksize; i++)
                *out++ = *in++;
        }
    } else {
        u32 i;
        const i32* li = buffer[0];
        const i32* ri = buffer[1];

        if (ff->info->bits == 8) {
            char* lo = (char*) ff->buffer + 0;
            char* ro = (char*) ff->buffer + 1;
            for (i = 0; i < frame->header.blocksize; i++, lo++, ro++) {
                *lo++ = *li++ + 128;
                *ro++ = *ri++ + 128;
            }
        } else {
            i16* lo = (i16*) ff->buffer + 0;
            i16* ro = (i16*) ff->buffer + 1;
            for (i = 0; i < frame->header.blocksize; i++, lo++, ro++) {
                *lo++ = *li++;
                *ro++ = *ri++;
            }
        }
    }

    ff->size = frame->header.blocksize * ff->info->width * ff->info->channels;
    ff->pos = 0;
    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

static void FLAC_Meta_f(
    const FLAC__StreamDecoder* decoder,
    const FLAC__StreamMetadata* metadata,
    void* client_data
) {
    if (metadata->type != FLAC__METADATA_TYPE_STREAMINFO) {
        return;
    }
    const flacfile_t* ff = client_data;
    snd_info_t* info = ff->info;
    const FLAC__StreamMetadata_StreamInfo* stream = &metadata->data.stream_info;
    info->rate = (i32) stream->sample_rate;
    info->bits = (i32) stream->bits_per_sample;
    info->width = info->bits / 8;
    info->channels = (i32) stream->channels;
    info->blocksize = (i32) stream->max_blocksize;
    info->dataofs = 0; // got the STREAMINFO metadata
}


static qboolean S_FLAC_CodecInitialize(void) {
    return true;
}

static void S_FLAC_CodecShutdown(void) {
}

static void FLAC_FreeFile(flacfile_t* ff) {
    if (ff->decoder) {
        FLAC__stream_decoder_finish(ff->decoder);
        FLAC__stream_decoder_delete(ff->decoder);
    }
    if (ff->buffer) {
        Q_free(ff->buffer);
    }
    Z_Free(ff);
}

static qboolean S_FLAC_CodecOpenStream(snd_stream_t* stream) {
    flacfile_t* ff = Z_Malloc(sizeof(flacfile_t));
    ff->buffer = NULL;

    ff->decoder = FLAC__stream_decoder_new();
    if (ff->decoder == NULL) {
        Con_Printf("Unable to create fLaC decoder\n");
        FLAC_FreeFile(ff);
        return false;
    }

    stream->priv = ff;
    ff->info = &stream->info;
    ff->file = &stream->fh;
    ff->info->dataofs = -1; // check for STREAMINFO metadata existence

    i32 rc = FLAC__stream_decoder_init_stream(
        ff->decoder,
        FLAC_Read_f,
        FLAC_Seek_f,
        FLAC_Tell_f,
        FLAC_Length_f,
        FLAC_EOF_f,
        FLAC_Write_f,
        FLAC_Meta_f,
        FLAC_Error_f,
        ff
    );
    if (rc != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
        Con_Printf("FLAC: decoder init error %i\n", rc);
        FLAC_FreeFile(ff);
        return false;
    }

    rc = FLAC__stream_decoder_process_until_end_of_metadata(ff->decoder);
    if (rc == false || ff->error) {
        rc = FLAC__stream_decoder_get_state(ff->decoder);
        Con_Printf("%s not a valid flac file? (decoder state %i)\n",
                   stream->name, rc);
        FLAC_FreeFile(ff);
        return false;
    }
    if (ff->info->dataofs < 0) {
        Con_Printf("%s has no STREAMINFO\n", stream->name);
        FLAC_FreeFile(ff);
        return false;
    }
    if (ff->info->bits != 8 && ff->info->bits != 16) {
        Con_Printf("%s is not 8 or 16 bit\n", stream->name);
        FLAC_FreeFile(ff);
        return false;
    }
    if (ff->info->channels != 1 && ff->info->channels != 2) {
        Con_Printf("Unsupported number of channels %d in %s\n",
                   ff->info->channels, stream->name);
        FLAC_FreeFile(ff);
        return false;
    }

    return true;
}

static i32 S_FLAC_CodecReadStream(snd_stream_t* stream, i32 len, void* buffer) {
    flacfile_t* ff = stream->priv;
    byte* buf = buffer;
    i32 count = 0;

    while (len) {
        if (ff->size == ff->pos) {
            FLAC__stream_decoder_process_single(ff->decoder);
        }
        if (ff->error) {
            return -1;
        }
        i32 res = ff->size - ff->pos;
        if (res > len) {
            res = len;
        }
        if (res < 0) {
            // Error
            return -1;
        }
        if (res == 0) {
            Con_DPrintf("FLAC: EOF\n");
            break;
        }
        Q_memcpy(buf, ff->buffer + ff->pos, res);
        count += res;
        len -= res;
        buf += res;
        ff->pos += res;
    }

    return count;
}

static void S_FLAC_CodecCloseStream(snd_stream_t* stream) {
    flacfile_t* ff = stream->priv;
    FLAC_FreeFile(ff);
    S_CodecUtilClose(&stream);
}

static i32 S_FLAC_CodecRewindStream(snd_stream_t* stream) {
    flacfile_t* ff = stream->priv;
    ff->pos = ff->size = 0;
    if (FLAC__stream_decoder_seek_absolute(ff->decoder, 0)) {
        return 0;
    }
    return -1;
}

snd_codec_t flac_codec = {
    .type = CODECTYPE_FLAC,
    .initialized = true, // always available.
    .ext = "flac",
    .initialize = S_FLAC_CodecInitialize,
    .shutdown = S_FLAC_CodecShutdown,
    .codec_open = S_FLAC_CodecOpenStream,
    .codec_read = S_FLAC_CodecReadStream,
    .codec_rewind = S_FLAC_CodecRewindStream,
    .codec_jump = NULL,
    .codec_close = S_FLAC_CodecCloseStream,
    .next = NULL
};
