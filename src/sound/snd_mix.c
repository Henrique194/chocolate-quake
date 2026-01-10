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
// snd_mix.c -- portable code to mix sounds for snd_dma.c


#include "sound.h"
#include <SDL_stdinc.h>
#include <stdlib.h>


#define PAINTBUFFER_SIZE 2048
portable_samplepair_t paintbuffer[PAINTBUFFER_SIZE];
i32 snd_scaletable[32][256];
i32 *snd_p, snd_linear_count;
i16* snd_out;

static i32 snd_vol;

typedef struct {
    float* memory;  // kernelsize floats
    float* kernel;  // kernelsize floats
    i32 kernelsize; // M+1, rounded up to be a multiple of 16
    i32 M;          // M value used to make kernel, even
    i32 parity;     // 0-3
    float f_c;      // cutoff frequency, [0..1], fraction of sample rate
} filter_t;


static void Snd_WriteLinearBlastStereo16(void) {
    i32 i;
    i32 val;

    for (i = 0; i < snd_linear_count; i += 2) {
        val = snd_p[i] / 256;
        if (val > 0x7fff)
            snd_out[i] = 0x7fff;
        else if (val < (i16) 0x8000)
            snd_out[i] = (i16) 0x8000;
        else
            snd_out[i] = val;

        val = snd_p[i + 1] / 256;
        if (val > 0x7fff)
            snd_out[i + 1] = 0x7fff;
        else if (val < (i16) 0x8000)
            snd_out[i + 1] = (i16) 0x8000;
        else
            snd_out[i + 1] = val;
    }
}

static void S_TransferStereo16(i32 endtime) {
    i32 lpos;
    i32 lpaintedtime;

    snd_p = (i32*) paintbuffer;
    lpaintedtime = paintedtime;

    while (lpaintedtime < endtime) {
        // handle recirculating buffer issues
        lpos = lpaintedtime & ((shm->samples >> 1) - 1);

        snd_out = (i16*) shm->buffer + (lpos << 1);

        snd_linear_count = (shm->samples >> 1) - lpos;
        if (lpaintedtime + snd_linear_count > endtime)
            snd_linear_count = endtime - lpaintedtime;

        snd_linear_count <<= 1;

        // write a linear blast of samples
        Snd_WriteLinearBlastStereo16();

        snd_p += snd_linear_count;
        lpaintedtime += (snd_linear_count >> 1);
    }
}

static void S_TransferPaintBuffer(i32 endtime) {
    i32 out_idx, out_mask;
    i32 count, step, val;
    i32* p;

    if (shm->samplebits == 16 && shm->channels == 2) {
        S_TransferStereo16(endtime);
        return;
    }

    p = (i32*) paintbuffer;
    count = (endtime - paintedtime) * shm->channels;
    out_mask = shm->samples - 1;
    out_idx = paintedtime * shm->channels & out_mask;
    step = 3 - shm->channels;

    if (shm->samplebits == 16) {
        i16* out = (i16*) shm->buffer;
        while (count--) {
            val = *p / 256;
            p += step;
            if (val > 0x7fff)
                val = 0x7fff;
            else if (val < (i16) 0x8000)
                val = (i16) 0x8000;
            out[out_idx] = val;
            out_idx = (out_idx + 1) & out_mask;
        }
    } else if (shm->samplebits == 8 && !shm->signed8) {
        byte* out = shm->buffer;
        while (count--) {
            val = *p / 256;
            p += step;
            if (val > 0x7fff)
                val = 0x7fff;
            else if (val < (i16) 0x8000)
                val = (i16) 0x8000;
            out[out_idx] = (val / 256) + 128;
            out_idx = (out_idx + 1) & out_mask;
        }
    } else if (shm->samplebits == 8) {
        /* S8 format, e.g. with Amiga AHI */
        i8* out = (i8*) shm->buffer;
        while (count--) {
            val = *p / 256;
            p += step;
            if (val > 0x7fff)
                val = 0x7fff;
            else if (val < (i16) 0x8000)
                val = (i16) 0x8000;
            out[out_idx] = (val / 256);
            out_idx = (out_idx + 1) & out_mask;
        }
    }
}

/*
============
S_NormalizeKernel

Normalize the filter kernel so the filter has unity gain
============
*/
static void S_NormalizeKernel(float* kernel, i32 M) {
    float sum = 0;
    for (i32 i = 0; i <= M; i++) {
        sum += kernel[i];
    }
    for (i32 i = 0; i <= M; i++) {
        kernel[i] /= sum;
    }
}

/*
==============
S_MakeWindowedKernel

Makes a lowpass filter kernel, from equation 16-4 in
"The Scientist and Engineer's Guide to Digital Signal Processing"

M is the kernel size (not counting the center point), must be even
kernel has room for M+1 floats
f_c is the filter cutoff frequency, as a fraction of the samplerate
==============
*/
static float* S_MakeWindowedKernel(i32 kernelsize, i32 M, float f_c) {
    float* kernel = (float*) Q_calloc(kernelsize, sizeof(*kernel));
    if (!kernel) {
        return NULL;
    }

    // The ideal filter kernel defined by the sinc function has infinite
    // length, which is a problem to computers. So we truncate the sinc
    // function to M + 1 points (from 0 to M). Then, to smooth the truncated
    // sinc function, we multiply by the Blackman window.
    double mf = (double) M;
    double tau = 2 * M_PI;
    for (i32 i = 0; i <= M; i++) {
        if (i == M / 2) {
            // Handle the center point explicitly to avoid division by zero.
            kernel[i] = (float) (tau * f_c);
            continue;
        }
        // Shift index so that the sinc function is centered at M/2.
        double kernel_shift = i - (M / 2.0);
        // Ideal sinc function value for this index.
        double sinc = sin(tau * f_c * kernel_shift) / kernel_shift;
        // Blackman window function to smooth the truncation.
        double window = 0.42;
        window -= (0.5 * cos(tau * i / mf));
        window += (0.08 * cos(2 * tau * i / mf));

        kernel[i] = (float) (sinc * window);
    }
    S_NormalizeKernel(kernel, M);

    return kernel;
}

static void S_UpdateFilter(filter_t* filter, i32 M, float f_c) {
    if (filter->f_c == f_c && filter->M == M) {
        // No need to update if cutoff frequency and
        // filter length are unchanged.
        return;
    }
    if (filter->memory != NULL) {
        Q_free(filter->memory);
    }
    if (filter->kernel != NULL) {
        Q_free(filter->kernel);
    }
    filter->M = M;
    filter->f_c = f_c;
    filter->parity = 0;
    // M + 1 rounded up to the next multiple of 16
    filter->kernelsize = (M + 1) + 16 - ((M + 1) % 16);
    filter->memory = (float*) Q_calloc(filter->kernelsize, sizeof(float));
    filter->kernel = S_MakeWindowedKernel(filter->kernelsize, M, f_c);
}

/*
==============
S_ApplyFilter

Lowpass-filter the given buffer containing 44100Hz audio.

As an optimization, it decimates the audio to 11025Hz (setting every sample
position that's not a multiple of 4 to 0), then convoluting with the filter
kernel is 4x faster, because we can skip 3/4 of the input samples that are
known to be 0 and skip 3/4 of the filter kernel.
==============
*/
static void S_ApplyFilter(filter_t* filter, i32* data, i32 stride, i32 count) {
    // Set up the input buffer.
    // Memory holds the previous filter->kernelsize samples of input.
    float* input =
        (float*) Q_malloc(sizeof(float) * (filter->kernelsize + count));
    if (!input) {
        return;
    }
    Q_memcpy(input, filter->memory, filter->kernelsize * sizeof(float));
    for (i32 i = 0; i < count; i++) {
        input[filter->kernelsize + i] =
            (float) data[i * stride] / (32768.0f * 256.0f);
    }

    // Copy out the last filter->kernelsize samples to 'memory' for next time.
    Q_memcpy(filter->memory, input + count, filter->kernelsize * sizeof(float));

    // Apply the filter
    i32 parity = filter->parity;
    const i32 kernelsize = filter->kernelsize;
    const float* kernel = filter->kernel;
    for (i32 i = 0; i < count; i++) {
        const float* input_plus_i = input + i;
        float val[4] = {0, 0, 0, 0};
        for (i32 j = (4 - parity) % 4; j < kernelsize; j += 16) {
            val[0] += kernel[j] * input_plus_i[j];
            val[1] += kernel[j + 4] * input_plus_i[j + 4];
            val[2] += kernel[j + 8] * input_plus_i[j + 8];
            val[3] += kernel[j + 12] * input_plus_i[j + 12];
        }
        // 4.0 factor is to increase volume by 12 dB; this is to make up the
        // volume drop caused by the zero-filling this filter does.
        float valf =
            (val[0] + val[1] + val[2] + val[3]) * (32768.0f * 256.0f * 4.0f);
        data[i * stride] = (i32) valf;

        parity = (parity + 1) % 4;
    }
    filter->parity = parity;

    Q_free(input);
}

static void S_GetLowPassFilterInfo(i32* M, float* bw, float* fc) {
    i32 filter_quality = (i32) snd_filterquality.value;
    if (filter_quality < 1 || filter_quality > 5) {
        // If invalid quality, set it to max quality.
        filter_quality = 5;
    }
    *M = 102 + (filter_quality * 24);
    *bw = 0.885f + ((float) filter_quality * 0.015f);
    *fc = (*bw * 11025 / 2.0f) / 44100.0f;
}

/*
==============
S_LowpassFilter

lowpass filters 24-bit integer samples in 'data' (stored in 32-bit ints).
assumes 44100Hz sample rate, and lowpasses at around 5kHz
memory should be a zero-filled filter_t struct
==============
*/
static void S_LowPassFilter(i32* data, i32 stride, i32 count,
                            filter_t* memory) {
    i32 M;
    float bw;
    float fc;
    S_GetLowPassFilterInfo(&M, &bw, &fc);

    S_UpdateFilter(memory, M, fc);
    S_ApplyFilter(memory, data, stride, count);
}

/*
===============================================================================

CHANNEL MIXING

===============================================================================
*/

static void SND_PaintChannelFrom8(channel_t* ch, sfxcache_t* sc, i32 count,
                                  i32 paintbufferstart);
static void SND_PaintChannelFrom16(channel_t* ch, sfxcache_t* sc, i32 count,
                                   i32 paintbufferstart);

static void S_PaintMusic(i32 end) {
    // Copy from the streaming sound source.
    i32 stop = SDL_min(end, s_rawend);
    for (i32 i = paintedtime; i < stop; i++) {
        i32 s = i & (MAX_RAW_SAMPLES - 1);
        const portable_samplepair_t* raw_sample = &s_rawsamples[s];
        portable_samplepair_t* sample = &paintbuffer[i - paintedtime];
        // Lower music by 6db to match sfx.
        sample->left += (raw_sample->left / 2);
        sample->right += (raw_sample->right / 2);
    }
}

static void S_ApplyLowPassFilter(i32 end) {
    static filter_t memory_l;
    static filter_t memory_r;

    i32* data = (i32*) paintbuffer;
    i32 stride = 2;
    i32 count = end - paintedtime;
    S_LowPassFilter(data, stride, count, &memory_l);
    S_LowPassFilter(data + 1, stride, count, &memory_r);
}

//
// Clip each sample to 0dB, then reduce by 6dB (to leave some
// headroom for the lowpass filter and the music). The lowpass
// will smooth out the clipping.
//
static void S_ClipSamples(i32 end) {
    i32 min = -32768 * 256;
    i32 max = 32767 * 256;
    for (i32 i = 0; i < end - paintedtime; i++) {
        portable_samplepair_t* sample = &paintbuffer[i];
        sample->left = SDL_clamp(sample->left, min, max) / 2;
        sample->right = SDL_clamp(sample->right, min, max) / 2;
    }
}

//
// Paint channel up to end.
//
static void S_PaintSfxChannel(channel_t* ch, sfxcache_t* sc, i32 end) {
    i32 ltime = paintedtime;

    // paint up to end
    while (ltime < end) {
        i32 count = (ch->end < end) ? (ch->end - ltime) : (end - ltime);

        if (count > 0) {
            // the last param to SND_PaintChannelFrom is the index
            // to start painting to in the paintbuffer, usually 0.
            i32 start = ltime - paintedtime;
            if (sc->width == 1) {
                SND_PaintChannelFrom8(ch, sc, count, start);
            } else {
                SND_PaintChannelFrom16(ch, sc, count, start);
            }
            ltime += count;
        }

        // if at end of loop, restart
        if (ltime >= ch->end) {
            if (sc->loopstart >= 0) {
                ch->pos = sc->loopstart;
                ch->end = ltime + sc->length - ch->pos;
            } else {
                // channel just stopped
                ch->sfx = NULL;
                break;
            }
        }
    }
}

static void S_PaintSfx(i32 end) {
    for (i32 i = 0; i < total_channels; i++) {
        channel_t* ch = &snd_channels[i];
        if (!ch->sfx) {
            continue;
        }
        if (!ch->leftvol && !ch->rightvol) {
            continue;
        }
        sfxcache_t* sc = S_LoadSound(ch->sfx);
        if (sc) {
            S_PaintSfxChannel(ch, sc, end);
        }
    }
}

static void S_ClearPaintBuffer(i32 end) {
    size_t size = (end - paintedtime) * sizeof(*paintbuffer);
    Q_memset(paintbuffer, 0, size);
}

void S_PaintChannels(i32 endtime) {
    snd_vol = (i32) (sfxvolume.value * 256);

    while (paintedtime < endtime) {
        // If paintbuffer is smaller than DMA buffer.
        i32 end = SDL_min(endtime, paintedtime + PAINTBUFFER_SIZE);

        S_ClearPaintBuffer(end);
        S_PaintSfx(end);
        S_ClipSamples(end);
        if (sndspeed.value == 11025 && shm->speed == 44100) {
            S_ApplyLowPassFilter(end);
        }
        if (s_rawend >= paintedtime) {
            S_PaintMusic(end);
        }
        // Transfer out according to DMA format.
        S_TransferPaintBuffer(end);

        paintedtime = end;
    }
}

void SND_InitScaletable(void) {
    i32 i, j;
    i32 scale;

    for (i = 0; i < 32; i++) {
        scale = (i32) ((float) i * 8.0f * 256.0f * sfxvolume.value);
        for (j = 0; j < 256; j++) {
            snd_scaletable[i][j] = ((i8) j) * scale;
        }
    }
}


static void SND_PaintChannelFrom8(channel_t* ch, sfxcache_t* sc, i32 count,
                                  i32 paintbufferstart) {
    i32 data;
    i32 *lscale, *rscale;
    byte* sfx;
    i32 i;

    if (ch->leftvol > 255)
        ch->leftvol = 255;
    if (ch->rightvol > 255)
        ch->rightvol = 255;

    lscale = snd_scaletable[ch->leftvol >> 3];
    rscale = snd_scaletable[ch->rightvol >> 3];
    sfx = (byte*) sc->data + ch->pos;

    for (i = 0; i < count; i++) {
        data = sfx[i];
        paintbuffer[paintbufferstart + i].left += lscale[data];
        paintbuffer[paintbufferstart + i].right += rscale[data];
    }

    ch->pos += count;
}

static void SND_PaintChannelFrom16(channel_t* ch, sfxcache_t* sc, i32 count,
                                   i32 paintbufferstart) {
    i32 data;
    i32 left, right;
    i32 leftvol, rightvol;
    i16* sfx;
    i32 i;

    leftvol = ch->leftvol * snd_vol;
    rightvol = ch->rightvol * snd_vol;
    leftvol /= 256;
    rightvol /= 256;
    sfx = (i16*) sc->data + ch->pos;

    for (i = 0; i < count; i++) {
        data = sfx[i];
        // this was causing integer overflow as observed in quakespasm
        // with the warpspasm mod moved >>8 to left/right volume above.
        //	left = (data * leftvol) >> 8;
        //	right = (data * rightvol) >> 8;
        left = data * leftvol;
        right = data * rightvol;
        paintbuffer[paintbufferstart + i].left += left;
        paintbuffer[paintbufferstart + i].right += right;
    }

    ch->pos += count;
}
