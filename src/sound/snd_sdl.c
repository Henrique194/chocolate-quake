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


#include "sound.h"
#include "console.h"
#include <SDL.h>


static int buffersize;


static void SDLCALL paint_audio(void* unused, Uint8* stream, int len) {
    if (!shm) {
        // Shouldn't happen, but just in case.
        memset(stream, 0, len);
        return;
    }

    int pos = (shm->samplepos * (shm->samplebits / 8));
    if (pos >= buffersize) {
        shm->samplepos = 0;
        pos = 0;
    }
    int tobufend = buffersize - pos; // bytes to buffer's end.
    int len1 = len;
    int len2 = 0;
    if (len1 > tobufend) {
        len1 = tobufend;
        len2 = len - len1;
    }

    memcpy(stream, shm->buffer + pos, len1);

    if (len2 <= 0) {
        shm->samplepos += (len1 / (shm->samplebits / 8));
    } else {
        // wraparound?
        memcpy(stream + len1, shm->buffer, len2);
        shm->samplepos = (len2 / (shm->samplebits / 8));
    }

    if (shm->samplepos >= buffersize) {
        shm->samplepos = 0;
    }
}

static void SNDDMA_PrintInfo(const SDL_AudioSpec* device) {
    char drivername[128];

    Con_Printf("SDL audio spec  : %d Hz, %d samples, %d channels\n",
               device->freq, device->samples, device->channels);

    const char* driver_name = SDL_GetCurrentAudioDriver();
    const char* device_name = SDL_GetAudioDeviceName(0, SDL_FALSE);
    driver_name = driver_name ? driver_name : "(UNKNOWN)";
    device_name = device_name ? device_name : "(UNKNOWN)";
    snprintf(drivername, sizeof(drivername), "%s - %s", driver_name,
             device_name);
    Con_Printf("SDL audio driver: %s, %d bytes buffer\n", drivername,
               buffersize);
}

/*
============
SNDDMA_SetupDevice

Fill the audio DMA information block.
============
*/
static qboolean SNDDMA_InitAudio(dma_t* dma, const SDL_AudioSpec* device) {
    memset((void*) dma, 0, sizeof(dma_t));
    shm = dma;

    // First byte of format is bits.
    shm->samplebits = (device->format & 0xFF);
    shm->signed8 = (device->format == AUDIO_S8);
    shm->speed = device->freq;
    shm->channels = device->channels;
    shm->samplepos = 0;
    shm->submission_chunk = 1;
    shm->samples = (device->samples * device->channels) * 10;
    if (shm->samples & (shm->samples - 1)) {
        // Make it a power of two.
        int val = 1;
        while (val < shm->samples) {
            val <<= 1;
        }
        shm->samples = val;
    }

    buffersize = shm->samples * (shm->samplebits / 8);
    shm->buffer = (unsigned char*) calloc(1, buffersize);
    if (!shm->buffer) {
        SDL_CloseAudio();
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        shm = NULL;
        Con_Printf("Failed allocating memory for SDL audio\n");
        return false;
    }

    return true;
}

/*
============
SNDDMA_SetupDevice

Set up the desired device format.
============
*/
static void SNDDMA_SetupDevice(SDL_AudioSpec* device) {
    device->freq = (int) snd_mixspeed.value;
    device->format = (loadas8bit.value != 0) ? AUDIO_U8 : AUDIO_S16SYS;
    device->channels = 2; // desired_channels
    device->callback = paint_audio;
    device->userdata = NULL;
    if (device->freq <= 11025) {
        device->samples = 256;
    } else if (device->freq <= 22050) {
        device->samples = 512;
    } else if (device->freq <= 44100) {
        device->samples = 1024;
    } else if (device->freq <= 56000) {
        // for 48 kHz
        device->samples = 2048;
    } else {
        // for 96 kHz
        device->samples = 4096;
    }
}

/*
============
SNDDMA_OpenDevice

Set up the desired device format.
The audio data passed to the callback function will be guaranteed
to be in the requested format, and will be automatically converted
to the actual hardware audio format if necessary.
============
*/
static qboolean SNDDMA_OpenDevice(SDL_AudioSpec* device) {
    SNDDMA_SetupDevice(device);
    return SDL_OpenAudio(device, NULL) != -1;
}

qboolean SNDDMA_Init(dma_t* dma) {
    SDL_AudioSpec desired;
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
        Con_Printf("Couldn't init SDL audio: %s\n", SDL_GetError());
        return false;
    }
    if (!SNDDMA_OpenDevice(&desired)) {
        Con_Printf("Couldn't open SDL audio: %s\n", SDL_GetError());
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return false;
    }
    if (!SNDDMA_InitAudio(dma, &desired)) {
        return false;
    }
    SNDDMA_PrintInfo(&desired);
    SDL_PauseAudio(0);
    return true;
}

int SNDDMA_GetDMAPos(void) {
    return shm->samplepos;
}

void SNDDMA_Shutdown(void) {
    if (!shm) {
        return;
    }
    Con_Printf("Shutting down SDL sound\n");
    SDL_CloseAudio();
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    if (shm->buffer) {
        free(shm->buffer);
    }
    shm->buffer = NULL;
    shm = NULL;
}

void SNDDMA_LockBuffer(void) {
    SDL_LockAudio();
}

void SNDDMA_Submit(void) {
    SDL_UnlockAudio();
}

void SNDDMA_BlockSound(void) {
    SDL_PauseAudio(1);
}

void SNDDMA_UnblockSound(void) {
    SDL_PauseAudio(0);
}
