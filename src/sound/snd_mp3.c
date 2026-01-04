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

#include "snd_mp3.h"
#include "console.h"
#include <mad.h>
#include <SDL_endian.h>
#include <SDL_stdinc.h>


// Under Windows, importing data from DLLs is a dicey proposition.
// This is true when using dlopen, but also true if linking directly
// against the DLL if the header does not mark the data as
// __declspec(dllexport), which mad.h does not. Sidestep the issue by
// defining our own mad_timer_zero. This is needed because mad_timer_zero
// is used in some of the mad.h macros.
#define mad_timer_zero mad_timer_zero_stub
static mad_timer_t const mad_timer_zero_stub = {0, 0};

// MAD returns values with MAD_F_FRACBITS (28) bits of precision,
// though it's not certain that all of them are meaningful. Default
// to 16 bits to align with most users expectation of output file
// should be 16 bits.
#define MP3_MAD_SAMPLEBITS  16
#define MP3_MAD_SAMPLEWIDTH 2
#define MP3_BUFFER_SIZE     (5 * 8192)

// Private data
typedef struct {
    unsigned char mp3_buffer[MP3_BUFFER_SIZE];
    struct mad_stream stream;
    struct mad_frame frame;
    struct mad_synth synth;
    mad_timer_t timer;
    ptrdiff_t cur_samp;
    size_t frame_count;
} mp3_priv_t;


//
// (Re)fill the stream buffer that is to be decoded.
// If any data still exists in the buffer then they are
// first shifted to be front of the stream buffer.
//
static int MP3_InputData(snd_stream_t* stream) {
    mp3_priv_t* p = stream->priv;
    size_t bytes_read;
    size_t remaining;

    remaining = p->stream.bufend - p->stream.next_frame;

    // libmad does not consume all the buffer it's given.
    // Some data, part of a truncated frame, is left unused at the
    // end of the buffer. That data must be put back at the
    // beginning of the buffer and taken in account for
    // refilling the buffer. This means that the input buffer
    // must be large enough to hold a complete frame at the
    // highest observable bit-rate (currently 448 kb/s).
    // Is 2016 bytes the size of the largest frame? (448000*(1152/32000))/8
    memmove(p->mp3_buffer, p->stream.next_frame, remaining);

    bytes_read = Q_fread(p->mp3_buffer + remaining, 1,
                          MP3_BUFFER_SIZE - remaining, &stream->fh);
    if (bytes_read == 0)
        return -1;

    mad_stream_buffer(&p->stream, p->mp3_buffer, bytes_read + remaining);
    p->stream.error = MAD_ERROR_NONE;

    return 0;
}

static int MP3_StartRead(snd_stream_t* stream) {
    mp3_priv_t* p = stream->priv;
    size_t ReadSize;

    mad_stream_init(&p->stream);
    mad_frame_init(&p->frame);
    mad_synth_init(&p->synth);
    mad_timer_reset(&p->timer);

    // Decode at least one valid frame to find out the input format.
    // The decoded frame will be saved off so that it can be processed later.
    ReadSize = Q_fread(p->mp3_buffer, 1, MP3_BUFFER_SIZE, &stream->fh);
    if (!ReadSize || Q_ferror(&stream->fh)) {
        return -1;
    }

    mad_stream_buffer(&p->stream, p->mp3_buffer, ReadSize);

    // Find a valid frame before starting up.
    // This makes sure that we have a valid MP3.
    p->stream.error = MAD_ERROR_NONE;
    while (mad_frame_decode(&p->frame, &p->stream)) {
        // Check whether input buffer needs a refill.
        if (p->stream.error == MAD_ERROR_BUFLEN) {
            if (MP3_InputData(stream) == -1) {
                // EOF with no valid data.
                return -1;
            }
            continue;
        }

        // We know that a valid frame hasn't been found yet,
        // so help libmad out and go back into frame seek mode.
        mad_stream_sync(&p->stream);
        p->stream.error = MAD_ERROR_NONE;
    }

    if (p->stream.error) {
        Con_Printf("MP3: No valid MP3 frame found\n");
        return -1;
    }

    switch (p->frame.header.mode) {
        case MAD_MODE_SINGLE_CHANNEL:
        case MAD_MODE_DUAL_CHANNEL:
        case MAD_MODE_JOINT_STEREO:
        case MAD_MODE_STEREO:
            stream->info.channels = MAD_NCHANNELS(&p->frame.header);
            break;
        default:
            Con_Printf("MP3: Cannot determine number of channels\n");
            return -1;
    }

    p->frame_count = 1;

    mad_timer_add(&p->timer, p->frame.header.duration);
    mad_synth_frame(&p->synth, &p->frame);
    stream->info.rate = p->synth.pcm.samplerate;
    stream->info.bits = MP3_MAD_SAMPLEBITS;
    stream->info.width = MP3_MAD_SAMPLEWIDTH;

    p->cur_samp = 0;

    return 0;
}

// Read up to len samples from p->Synth
// If needed, read some more MP3 data, decode them and synth them Place in buf[].
// Return number of samples read.
static int MP3_Decode(snd_stream_t* stream, byte* buf, int len) {
    mp3_priv_t* p = stream->priv;
    int donow, i, done = 0;
    mad_fixed_t sample;
    int chan, x;

    do {
        x = (p->synth.pcm.length - p->cur_samp) * stream->info.channels;
        donow = SDL_min(len, x);
        i = 0;
        while (i < donow) {
            for (chan = 0; chan < stream->info.channels; chan++) {
                sample = p->synth.pcm.samples[chan][p->cur_samp];
                // Convert from fixed to short, write in host-endian format.
                if (sample <= -MAD_F_ONE)
                    sample = -0x7FFF;
                else if (sample >= MAD_F_ONE)
                    sample = 0x7FFF;
                else
                    sample >>= (MAD_F_FRACBITS + 1 - 16);
                sample = SDL_SwapLE16(sample);
                *buf++ = sample & 0xFF;
                *buf++ = (sample >> 8) & 0xFF;
                i++;
            }
            p->cur_samp++;
        }

        len -= donow;
        done += donow;

        if (len == 0)
            break;

        // Check whether input buffer needs a refill.
        if (p->stream.error == MAD_ERROR_BUFLEN) {
            if (MP3_InputData(stream) == -1) {
                Con_DPrintf("mp3 EOF\n");
                break;
            }
        }

        if (mad_frame_decode(&p->frame, &p->stream)) {
            if (MAD_RECOVERABLE(p->stream.error)) {
                // to frame seek mode.
                mad_stream_sync(&p->stream);
                continue;
            } else {
                if (p->stream.error == MAD_ERROR_BUFLEN)
                    continue;
                else {
                    Con_Printf("MP3: unrecoverable frame level error (%s)\n",
                               mad_stream_errorstr(&p->stream));
                    break;
                }
            }
        }
        p->frame_count++;
        mad_timer_add(&p->timer, p->frame.header.duration);
        mad_synth_frame(&p->synth, &p->frame);
        p->cur_samp = 0;
    } while (1);

    return done;
}

static void MP3_StopRead(snd_stream_t* stream) {
    mp3_priv_t* p = stream->priv;
    mad_synth_finish(&p->Synth);
    mad_frame_finish(&p->frame);
    mad_stream_finish(&p->stream);
}

static int MP3_MadSeek(snd_stream_t* stream, unsigned long offset) {
    mp3_priv_t* p = stream->priv;
    size_t initial_bitrate = p->frame.header.bitrate;
    size_t consumed = 0;
    int vbr = 0; // Variable Bit Rate
    qboolean depadded = false;
    unsigned long to_skip_samples = 0;

    // Reset all.
    Q_rewind(&stream->fh);
    mad_timer_reset(&p->timer);
    p->frame_count = 0;

    // They were opened in StartRead
    mad_synth_finish(&p->Synth);
    mad_frame_finish(&p->frame);
    mad_stream_finish(&p->stream);

    mad_stream_init(&p->stream);
    mad_frame_init(&p->frame);
    mad_synth_init(&p->synth);

    offset /= stream->info.channels;
    to_skip_samples = offset;

    // Read data from the MP3 file.
    while (1)
    {
        int bytes_read, padding = 0;
        size_t leftover = p->stream.bufend - p->stream.next_frame;

        memcpy(p->mp3_buffer, p->stream.this_frame, leftover);
        bytes_read = Q_fread(p->mp3_buffer + leftover, (size_t) 1,
                              MP3_BUFFER_SIZE - leftover, &stream->fh);
        if (bytes_read <= 0) {
            Con_DPrintf(
                "seek failure. unexpected EOF (frames=%lu leftover=%lu)\n",
                (unsigned long) p->frame_count, (unsigned long) leftover);
            break;
        }
        for (; !depadded && padding < bytes_read && !p->mp3_buffer[padding];
             ++padding)
            ;
        depadded = true;
        mad_stream_buffer(&p->stream, p->mp3_buffer + padding,
                          leftover + bytes_read - padding);

        // Decode frame headers.
        while (1)
        {
            static unsigned short samples;
            p->stream.error = MAD_ERROR_NONE;

            // Not an audio frame.
            if (mad_header_decode(&p->frame.header, &p->stream) == -1) {
                if (p->stream.error == MAD_ERROR_BUFLEN) {
                    // Normal behaviour; get some more data from the file
                    break;
                }
                if (!MAD_RECOVERABLE(p->stream.error)) {
                    Con_DPrintf("unrecoverable MAD error\n");
                    break;
                }
                if (p->stream.error == MAD_ERROR_LOSTSYNC) {
                    Con_DPrintf("MAD lost sync\n");
                } else {
                    Con_DPrintf("recoverable MAD error\n");
                }
                continue;
            }

            consumed += p->stream.next_frame - p->stream.this_frame;
            vbr |= (p->frame.header.bitrate != initial_bitrate);

            samples = 32 * MAD_NSBSAMPLES(&p->frame.header);

            p->frame_count++;
            mad_timer_add(&p->timer, p->frame.header.duration);

            if (to_skip_samples <= samples) {
                mad_frame_decode(&p->frame, &p->stream);
                mad_synth_frame(&p->synth, &p->frame);
                p->cur_samp = to_skip_samples;
                return 0;
            } else
                to_skip_samples -= samples;

            // If not VBR, we can extrapolate frame size.
            if (p->frame_count == 64 && !vbr) {
                p->frame_count = offset / samples;
                to_skip_samples = offset % samples;
                if (0 != Q_fseek(&stream->fh, (p->frame_count * consumed / 64), SEEK_SET)) {
                    return -1;
                }

                // Reset Stream for refilling buffer.
                mad_stream_finish(&p->stream);
                mad_stream_init(&p->stream);
                break;
            }
        }
    }

    return -1;
}

static qboolean S_MP3_CodecInitialize(void) {
    return true;
}

static void S_MP3_CodecShutdown(void) {
    // Nothing to shut down.
}

static qboolean S_MP3_CodecOpenStream(snd_stream_t* stream) {
    if (MP3_SkipTags(stream) < 0) {
        Con_Printf("Corrupt mp3 file (bad tags.)\n");
        return false;
    }

    stream->priv = calloc(1, sizeof(mp3_priv_t));
    if (!stream->priv) {
        Con_Printf("Insufficient memory for MP3 audio\n");
        return false;
    }
    const int err = MP3_StartRead(stream);
    if (err != 0) {
        Con_Printf("%s is not a valid mp3 file\n", stream->name);
    } else if (stream->info.channels != 1 && stream->info.channels != 2) {
        Con_Printf("Unsupported number of channels %d in %s\n",
                   stream->info.channels, stream->name);
    } else {
        return true;
    }
    free(stream->priv);
    return false;
}

static int S_MP3_CodecReadStream(
    snd_stream_t* stream,
    int bytes,
    void* buffer
) {
    const int len = bytes / stream->info.width;
    const int res = MP3_Decode(stream, buffer, len);
    return res * stream->info.width;
}

static void S_MP3_CodecCloseStream(snd_stream_t* stream) {
    MP3_StopRead(stream);
    free(stream->priv);
    S_CodecUtilClose(&stream);
}

static int S_MP3_CodecRewindStream(snd_stream_t* stream) {
    return MP3_MadSeek(stream, 0);
}

snd_codec_t mp3_codec = {
    .type = CODECTYPE_MP3,
    .initialized = true, // always available.
    .ext = "mp3",
    .initialize = S_MP3_CodecInitialize,
    .shutdown = S_MP3_CodecShutdown,
    .codec_open = S_MP3_CodecOpenStream,
    .codec_read = S_MP3_CodecReadStream,
    .codec_rewind = S_MP3_CodecRewindStream,
    .codec_jump = NULL,
    .codec_close = S_MP3_CodecCloseStream,
    .next = NULL
};
