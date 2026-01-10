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
// sound.h -- client sound i/o functions

#ifndef __SOUND__
#define __SOUND__

#include "quakedef.h"
#include "cvar.h"
#include "mathlib.h"
#include "zone.h"

#define DEFAULT_SOUND_PACKET_VOLUME      255
#define DEFAULT_SOUND_PACKET_ATTENUATION 1.0
#define WAV_FORMAT_PCM 1

// !!! if this is changed, it much be changed in asm_i386.h too !!!
typedef struct {
    i32 left;
    i32 right;
} portable_samplepair_t;

typedef struct sfx_s {
    char name[MAX_QPATH];
    cache_user_t cache;
} sfx_t;

// !!! if this is changed, it much be changed in asm_i386.h too !!!
typedef struct {
    i32 length;
    i32 loopstart;
    i32 speed;
    i32 width;
    i32 stereo;
    byte data[1]; // variable sized
} sfxcache_t;

typedef struct {
    qboolean gamealive;
    qboolean soundalive;
    qboolean splitbuffer;
    i32 channels;
    i32 samples;          // mono samples in buffer
    i32 submission_chunk; // don't mix less than this #
    i32 samplepos;        // in mono samples
    i32 samplebits;
    i32 signed8;
    i32 speed;
    byte* buffer;
} dma_t;

// !!! if this is changed, it much be changed in asm_i386.h too !!!
typedef struct {
    sfx_t* sfx;      // sfx number
    i32 leftvol;     // 0-255 volume
    i32 rightvol;    // 0-255 volume
    i32 end;         // end time in global paintsamples
    i32 pos;         // sample position in sfx
    i32 looping;     // where to loop, -1 = no looping
    i32 entnum;      // to allow overriding a specific sound
    i32 entchannel;  //
    vec3_t origin;   // origin of sound effect
    vec_t dist_mult; // distance multiplier (attenuation/clipK)
    i32 master_vol;  // 0-255 master volume
} channel_t;

typedef struct {
    i32 rate;
    i32 width;
    i32 channels;
    i32 loopstart;
    i32 samples;
    i32 dataofs; // chunk starts this many bytes from file start
} wavinfo_t;

void S_Init(void);
void S_Startup(void);
void S_Shutdown(void);
void S_StartSound(i32 entnum, i32 entchannel, sfx_t* sfx, vec3_t origin,
                  float fvol, float attenuation);
void S_StaticSound(sfx_t* sfx, vec3_t origin, float vol, float attenuation);
void S_StopSound(i32 entnum, i32 entchannel);
void S_StopAllSounds(qboolean clear);
void S_ClearBuffer(void);
void S_Update(vec3_t origin, vec3_t v_forward, vec3_t v_right, vec3_t v_up);
void S_ExtraUpdate(void);

sfx_t* S_PrecacheSound(char* sample);
void S_TouchSound(char* sample);
void S_ClearPrecache(void);
void S_BeginPrecaching(void);
void S_EndPrecaching(void);
void S_PaintChannels(i32 endtime);
void S_InitPaintChannels(void);

// picks a channel based on priorities, empty slots, number of channels
channel_t* SND_PickChannel(i32 entnum, i32 entchannel);

// spatializes a channel
void SND_Spatialize(channel_t* ch);

void S_RawSamples(i32 samples, i32 rate, i32 width, i32 channels, byte* data,
                  float volume);

// initializes cycling through a DMA buffer and returns information on it
qboolean SNDDMA_Init(dma_t* dma);

// gets the current DMA position
i32 SNDDMA_GetDMAPos(void);

// shutdown the DMA xfer.
void SNDDMA_Shutdown(void);

// ====================================================================
// User-setable variables
// ====================================================================

#define MAX_CHANNELS         128
#define MAX_DYNAMIC_CHANNELS 8
#define MAX_RAW_SAMPLES      8192


extern channel_t channels[MAX_CHANNELS];
// 0 to MAX_DYNAMIC_CHANNELS-1	= normal entity sounds
// MAX_DYNAMIC_CHANNELS to MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS -1 = water, etc
// MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS to total_channels = static sounds

extern i32 total_channels;

//
// Fake dma is a synchronous faking of the DMA progress used for
// isolating performance in the renderer.  The fakedma_updates is
// number of times S_Update() is called per second.
//

extern qboolean fakedma;
extern i32 fakedma_updates;
extern i32 paintedtime;
extern i32 s_rawend;
extern portable_samplepair_t s_rawsamples[MAX_RAW_SAMPLES];
extern vec3_t listener_origin;
extern vec3_t listener_forward;
extern vec3_t listener_right;
extern vec3_t listener_up;
extern volatile dma_t* shm;
extern volatile dma_t sn;
extern vec_t sound_nominal_clip_dist;

extern cvar_t sndspeed;
extern cvar_t snd_mixspeed;
extern cvar_t snd_filterquality;
//extern cvar_t sfxvolume;
extern cvar_t loadas8bit;
extern cvar_t bgmvolume;
extern cvar_t sfxvolume;
extern channel_t snd_channels[MAX_CHANNELS];

extern qboolean snd_initialized;

extern i32 snd_blocked;

void S_LocalSound(char* s);
sfxcache_t* S_LoadSound(sfx_t* s);

wavinfo_t GetWavinfo(char* name, byte* wav, i32 wavlength);

void SND_InitScaletable(void);
void SNDDMA_LockBuffer();
void SNDDMA_BlockSound();
void SNDDMA_UnblockSound();
void SNDDMA_Submit(void);

void S_AmbientOff(void);
void S_AmbientOn(void);

#endif
