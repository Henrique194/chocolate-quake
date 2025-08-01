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
// snd_dma.c -- main control for any streaming sound output device


#include "quakedef.h"
#include "snd_codec.h"


static void S_Play(void);
static void S_PlayVol(void);
static void S_SoundList(void);
static void S_Update_(void);
static void GetSoundtime(void);
void S_StopAllSounds(qboolean clear);
static void S_StopAllSoundsC(void);

// =======================================================================
// Internal sound data & structures
// =======================================================================

channel_t snd_channels[MAX_CHANNELS];
int total_channels;

int snd_blocked = 0;
qboolean snd_initialized = false;

volatile dma_t sn;
volatile dma_t* shm = NULL;

vec3_t listener_origin;
vec3_t listener_forward;
vec3_t listener_right;
vec3_t listener_up;
float voicevolumescale = 1; //for audio ducking while speaking

#define sound_nominal_clip_dist 1000.0

int soundtime;   // sample PAIRS
int paintedtime; // sample PAIRS

int s_rawend;
portable_samplepair_t s_rawsamples[MAX_RAW_SAMPLES];


#define MAX_SFX 512
static sfx_t* known_sfx = NULL; // hunk allocated [MAX_SFX]
static int num_sfx;

static sfx_t* ambient_sfx[NUM_AMBIENTS];

static qboolean sound_started = false;

cvar_t bgmvolume = {"bgmvolume", "1", true};
cvar_t sfxvolume = {"volume", "0.7", true};
static float old_sfxvolume = -1.0f;

cvar_t precache = {"precache", "1", false};
cvar_t loadas8bit = {"loadas8bit", "0", false};

cvar_t sndspeed = {"sndspeed", "11025", false};
cvar_t snd_mixspeed = {"snd_mixspeed", "44100", true};

#if defined(_WIN32)
#define SND_FILTERQUALITY_DEFAULT "5"
#else
#define SND_FILTERQUALITY_DEFAULT "1"
#endif

cvar_t snd_filterquality = {"snd_filterquality", SND_FILTERQUALITY_DEFAULT,
                            false};
static float old_filterquality = -1.0f;

static cvar_t nosound = {"nosound", "0", false};
static cvar_t ambient_level = {"ambient_level", "0.3", false};
static cvar_t ambient_fade = {"ambient_fade", "100", false};
static cvar_t snd_noextraupdate = {"snd_noextraupdate", "0", false};
static cvar_t snd_show = {"snd_show", "0", false};
static cvar_t _snd_mixahead = {"_snd_mixahead", "0.1", true};


static void S_SoundInfo_f(void) {
    if (!sound_started || !shm) {
        Con_Printf("sound system not started\n");
        return;
    }

    Con_Printf("%d bit, %s, %d Hz\n", shm->samplebits,
               (shm->channels == 2) ? "stereo" : "mono", shm->speed);
    Con_Printf("%5d samples\n", shm->samples);
    Con_Printf("%5d samplepos\n", shm->samplepos);
    Con_Printf("%5d submission_chunk\n", shm->submission_chunk);
    Con_Printf("%5d total_channels\n", total_channels);
    Con_Printf("%p dma buffer\n", shm->buffer);
}


static void SND_UpdateSfxVolume() {
    old_sfxvolume = sfxvolume.value;
    SND_InitScaletable();
}

static void SND_UpdateFilterQuality() {
    if (snd_filterquality.value < 1 || snd_filterquality.value > 5) {
        Con_Printf("snd_filterquality must be between 1 and 5\n");
        Cvar_Set("snd_filterquality", SND_FILTERQUALITY_DEFAULT);
    }
    old_filterquality = snd_filterquality.value;
}

/*
================
S_Startup
================
*/
void S_Startup(void) {
    if (!snd_initialized) {
        return;
    }
    sound_started = SNDDMA_Init(&sn);
    if (!sound_started) {
        Con_Printf("Failed initializing sound\n");
        return;
    }
    Con_Printf("Audio: %d bit, %s, %d Hz\n", shm->samplebits,
               (shm->channels == 2) ? "stereo" : "mono", shm->speed);
    GetSoundtime();
    paintedtime = soundtime;
}


static void S_PrecacheAmbientSound() {
    ambient_sfx[AMBIENT_WATER] = S_PrecacheSound("ambience/water1.wav");
    ambient_sfx[AMBIENT_SKY] = S_PrecacheSound("ambience/wind2.wav");
}

static void S_InitVariables() {
    int i = COM_CheckParm("-sndspeed");
    if (i && i < com_argc - 1) {
        Cvar_Set("sndspeed", com_argv[i + 1]);
    }

    i = COM_CheckParm("-mixspeed");
    if (i && i < com_argc - 1) {
        Cvar_Set("snd_mixspeed", com_argv[i + 1]);
    }

    if (host_parms.memsize < 0x800000) {
        Cvar_Set("loadas8bit", "1");
        Con_Printf("loading all sounds as 8bit\n");
    }

    SND_UpdateSfxVolume();
    SND_UpdateFilterQuality();
}

static void S_AddCommands(void) {
    Cmd_AddCommand("play", S_Play);
    Cmd_AddCommand("playvol", S_PlayVol);
    Cmd_AddCommand("stopsound", S_StopAllSoundsC);
    Cmd_AddCommand("soundlist", S_SoundList);
    Cmd_AddCommand("soundinfo", S_SoundInfo_f);
}

static void S_RegisterConsoleVars(void) {
    Cvar_RegisterVariable(&nosound);
    Cvar_RegisterVariable(&sfxvolume);
    Cvar_RegisterVariable(&precache);
    Cvar_RegisterVariable(&loadas8bit);
    Cvar_RegisterVariable(&bgmvolume);
    Cvar_RegisterVariable(&ambient_level);
    Cvar_RegisterVariable(&ambient_fade);
    Cvar_RegisterVariable(&snd_noextraupdate);
    Cvar_RegisterVariable(&snd_show);
    Cvar_RegisterVariable(&_snd_mixahead);

    Cvar_RegisterVariable(&sndspeed);
    Cvar_RegisterVariable(&snd_mixspeed);
    Cvar_RegisterVariable(&snd_filterquality);
}

/*
================
S_Init
================
*/
void S_Init(void) {
    Con_Printf("\nSound Initialization\n");
    if (snd_initialized) {
        Con_Printf("Sound is already initialized\n");
        return;
    }
    if (COM_CheckParm("-nosound")) {
        return;
    }
    S_RegisterConsoleVars();
    S_AddCommands();
    S_InitVariables();
    known_sfx = (sfx_t*) Hunk_AllocName(MAX_SFX * sizeof(sfx_t), "sfx_t");
    num_sfx = 0;
    snd_initialized = true;

    S_Startup();
    if (!sound_started) {
        return;
    }

    // provides a tick sound until washed clean
    //    if (shm->buffer) {
    //        shm->buffer[4] = shm->buffer[5] = 0x7f; // force a pop for debugging
    //    }

    S_PrecacheAmbientSound();
    S_CodecInit();
    S_StopAllSounds(true);
}


// =======================================================================
// Shutdown sound engine
// =======================================================================
void S_Shutdown(void) {
    if (!sound_started)
        return;

    sound_started = 0;
    snd_blocked = 0;

    S_CodecShutdown();

    SNDDMA_Shutdown();
    shm = NULL;
}


// =======================================================================
// Load a sound
// =======================================================================

/*
==================
S_FindName

==================
*/
static sfx_t* S_FindName(const char* name) {
    int i;
    sfx_t* sfx;

    if (!name)
        Sys_Error("S_FindName: NULL");

    if (strlen(name) >= MAX_QPATH)
        Sys_Error("Sound name too long: %s", name);

    // see if already loaded
    for (i = 0; i < num_sfx; i++) {
        if (!strcmp(known_sfx[i].name, name)) {
            return &known_sfx[i];
        }
    }

    if (num_sfx == MAX_SFX)
        Sys_Error("S_FindName: out of sfx_t");

    sfx = &known_sfx[i];
    Q_strncpy(sfx->name, name, sizeof(sfx->name));

    num_sfx++;

    return sfx;
}


/*
==================
S_TouchSound

==================
*/
void S_TouchSound(char* name) {
    sfx_t* sfx;

    if (!sound_started)
        return;

    sfx = S_FindName(name);
    Cache_Check(&sfx->cache);
}

/*
==================
S_PrecacheSound

==================
*/
sfx_t* S_PrecacheSound(char* name) {
    sfx_t* sfx;

    if (!sound_started || nosound.value)
        return NULL;

    sfx = S_FindName(name);

    // cache it in
    if (precache.value)
        S_LoadSound(sfx);

    return sfx;
}


//=============================================================================

/*
=================
SND_PickChannel

picks a channel based on priorities, empty slots, number of channels
=================
*/
channel_t* SND_PickChannel(int entnum, int entchannel) {
    int ch_idx;
    int first_to_die;
    int life_left;

    // Check for replacement sound, or find the best one to replace
    first_to_die = -1;
    life_left = 0x7fffffff;
    for (ch_idx = NUM_AMBIENTS; ch_idx < NUM_AMBIENTS + MAX_DYNAMIC_CHANNELS; ch_idx++) {
        if (entchannel != 0 // channel 0 never overrides
            && snd_channels[ch_idx].entnum == entnum
            && (snd_channels[ch_idx].entchannel == entchannel || entchannel == -1))
        { // always override sound from same entity
            first_to_die = ch_idx;
            break;
        }

        // don't let monster sounds override player sounds
        if (snd_channels[ch_idx].entnum == cl.viewentity
            && entnum != cl.viewentity
            && snd_channels[ch_idx].sfx)
            continue;

        if (snd_channels[ch_idx].end - paintedtime < life_left) {
            life_left = snd_channels[ch_idx].end - paintedtime;
            first_to_die = ch_idx;
        }
    }

    if (first_to_die == -1)
        return NULL;

    if (snd_channels[first_to_die].sfx)
        snd_channels[first_to_die].sfx = NULL;

    return &snd_channels[first_to_die];
}

/*
=================
SND_Spatialize

spatializes a channel
=================
*/
void SND_Spatialize(channel_t* ch) {
    vec_t dot;
    vec_t dist;
    vec_t lscale, rscale, scale;
    vec3_t source_vec;

    if (ch->entchannel == -2) {
        ch->leftvol = ch->master_vol; //voip comes out full volume
        ch->rightvol = ch->master_vol;
        return;
    }
    // anything coming from the view entity will always be full volume
    if (ch->entnum == cl.viewentity) {
        ch->leftvol = ch->master_vol * voicevolumescale;
        ch->rightvol = ch->master_vol * voicevolumescale;
        return;
    }

    // calculate stereo seperation and distance attenuation
    VectorSubtract(ch->origin, listener_origin, source_vec);
    dist = VectorNormalize(source_vec) * ch->dist_mult;
    dot = DotProduct(listener_right, source_vec);

    if (shm->channels == 1) {
        rscale = 1.0;
        lscale = 1.0;
    } else {
        rscale = 1.0 + dot;
        lscale = 1.0 - dot;
    }

    // add in distance effect
    scale = (1.0 - dist) * rscale;
    ch->rightvol = (int) (ch->master_vol * scale * voicevolumescale);
    if (ch->rightvol < 0)
        ch->rightvol = 0;

    scale = (1.0 - dist) * lscale;
    ch->leftvol = (int) (ch->master_vol * scale * voicevolumescale);
    if (ch->leftvol < 0)
        ch->leftvol = 0;
}


// =======================================================================
// Start a sound effect
// =======================================================================

void S_StartSound(int entnum, int entchannel, sfx_t* sfx, vec3_t origin,
                  float fvol, float attenuation) {
    channel_t *target_chan, *check;
    sfxcache_t* sc;
    int ch_idx;
    int skip;

    if (!sound_started)
        return;

    if (!sfx)
        return;

    if (nosound.value)
        return;

    // pick a channel to play on
    target_chan = SND_PickChannel(entnum, entchannel);
    if (!target_chan)
        return;

    // spatialize
    memset(target_chan, 0, sizeof(*target_chan));
    VectorCopy(origin, target_chan->origin);
    target_chan->dist_mult = attenuation / sound_nominal_clip_dist;
    target_chan->master_vol = (int) (fvol * 255);
    target_chan->entnum = entnum;
    target_chan->entchannel = entchannel;
    SND_Spatialize(target_chan);

    if (!target_chan->leftvol && !target_chan->rightvol)
        return; // not audible at all

    // new channel
    sc = S_LoadSound(sfx);
    if (!sc) {
        target_chan->sfx = NULL;
        return; // couldn't load the sound's data
    }

    target_chan->sfx = sfx;
    target_chan->pos = 0.0;
    target_chan->end = paintedtime + sc->length;

    // if an identical sound has also been started this frame, offset the pos
    // a bit to keep it from just making the first one louder
    check = &snd_channels[NUM_AMBIENTS];
    for (ch_idx = NUM_AMBIENTS; ch_idx < NUM_AMBIENTS + MAX_DYNAMIC_CHANNELS;
         ch_idx++, check++) {
        if (check == target_chan)
            continue;
        if (check->sfx == sfx && !check->pos) {
            /*
			skip = rand () % (int)(0.1 * shm->speed);
			if (skip >= target_chan->end)
				skip = target_chan->end - 1;
			*/
            /* LordHavoc: fixed skip calculations */
            skip = 0.1 * shm->speed; /* 0.1 * sc->speed */
            if (skip > sc->length)
                skip = sc->length;
            if (skip > 0)
                skip = rand() % skip;
            target_chan->pos += skip;
            target_chan->end -= skip;
            break;
        }
    }
}

void S_StopSound(int entnum, int entchannel) {
    int i;

    for (i = 0; i < MAX_DYNAMIC_CHANNELS; i++) {
        if (snd_channels[i].entnum == entnum
            && snd_channels[i].entchannel == entchannel) {
            snd_channels[i].end = 0;
            snd_channels[i].sfx = NULL;
            return;
        }
    }
}

void S_StopAllSounds(qboolean clear) {
    int i;
    if (!sound_started)
        return;

    total_channels = MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS; // no statics

    for (i = 0; i < MAX_CHANNELS; i++)
        if (snd_channels[i].sfx)
            snd_channels[i].sfx = NULL;

    memset(snd_channels, 0, MAX_CHANNELS * sizeof(channel_t));

    if (clear)
        S_ClearBuffer();
}

static void S_StopAllSoundsC(void) {
    S_StopAllSounds(true);
}

void S_ClearBuffer(void) {
    int clear;

    if (!sound_started || !shm)
        return;

    SNDDMA_LockBuffer();
    if (!shm->buffer)
        return;

    s_rawend = 0;

    if (shm->samplebits == 8 && !shm->signed8)
        clear = 0x80;
    else
        clear = 0;

    memset(shm->buffer, clear, shm->samples * shm->samplebits / 8);

    SNDDMA_Submit();
}


/*
=================
S_StaticSound
=================
*/
void S_StaticSound(sfx_t* sfx, vec3_t origin, float vol, float attenuation) {
    channel_t* ss;
    sfxcache_t* sc;

    if (!sfx)
        return;

    if (total_channels == MAX_CHANNELS) {
        Con_Printf("total_channels == MAX_CHANNELS\n");
        return;
    }

    ss = &snd_channels[total_channels];
    total_channels++;

    sc = S_LoadSound(sfx);
    if (!sc)
        return;

    if (sc->loopstart == -1) {
        Con_Printf("Sound %s not looped\n", sfx->name);
        return;
    }

    ss->sfx = sfx;
    VectorCopy(origin, ss->origin);
    ss->master_vol = (int) vol;
    ss->dist_mult = (attenuation / 64) / sound_nominal_clip_dist;
    ss->end = paintedtime + sc->length;

    SND_Spatialize(ss);
}


//=============================================================================

/*
===================
S_UpdateAmbientSounds
===================
*/
static void S_UpdateAmbientSounds(void) {
    mleaf_t* l;
    float vol;
    int ambient_channel;
    channel_t* chan;

    // calc ambient sound levels
    if (!cl.worldmodel)
        return;

    l = Mod_PointInLeaf(listener_origin, cl.worldmodel);
    if (!l || !ambient_level.value) {
        for (ambient_channel = 0; ambient_channel < NUM_AMBIENTS;
             ambient_channel++)
            snd_channels[ambient_channel].sfx = NULL;
        return;
    }

    for (ambient_channel = 0; ambient_channel < NUM_AMBIENTS;
         ambient_channel++) {
        chan = &snd_channels[ambient_channel];
        chan->sfx = ambient_sfx[ambient_channel];

        vol = ambient_level.value * l->ambient_sound_level[ambient_channel];
        if (vol < 8)
            vol = 0;

        // don't adjust volume too fast
        if (chan->master_vol < vol) {
            chan->master_vol += host_frametime * ambient_fade.value;
            if (chan->master_vol > vol)
                chan->master_vol = vol;
        } else if (chan->master_vol > vol) {
            chan->master_vol -= host_frametime * ambient_fade.value;
            if (chan->master_vol < vol)
                chan->master_vol = vol;
        }

        chan->leftvol = chan->rightvol = chan->master_vol;
    }
}


/*
===================
S_RawSamples		(from QuakeII)

Streaming music support. Byte swapping
of data must be handled by the codec.
Expects data in signed 16 bit, or unsigned
8 bit format.
===================
*/
void S_RawSamples(int samples, int rate, int width, int channels, byte* data,
                  float volume) {
    int i;
    int src, dst;
    float scale;
    int intVolume;

    if (s_rawend < paintedtime)
        s_rawend = paintedtime;

    scale = (float) rate / shm->speed;
    intVolume = (int) (256 * volume);

    if (channels == 2 && width == 2) {
        for (i = 0;; i++) {
            src = i * scale;
            if (src >= samples)
                break;
            dst = s_rawend & (MAX_RAW_SAMPLES - 1);
            s_rawend++;
            s_rawsamples[dst].left = ((short*) data)[src * 2] * intVolume;
            s_rawsamples[dst].right = ((short*) data)[src * 2 + 1] * intVolume;
        }
    } else if (channels == 1 && width == 2) {
        for (i = 0;; i++) {
            src = i * scale;
            if (src >= samples)
                break;
            dst = s_rawend & (MAX_RAW_SAMPLES - 1);
            s_rawend++;
            s_rawsamples[dst].left = ((short*) data)[src] * intVolume;
            s_rawsamples[dst].right = ((short*) data)[src] * intVolume;
        }
    } else if (channels == 2 && width == 1) {
        intVolume *= 256;

        for (i = 0;; i++) {
            src = i * scale;
            if (src >= samples)
                break;
            dst = s_rawend & (MAX_RAW_SAMPLES - 1);
            s_rawend++;
            //	s_rawsamples [dst].left = ((signed char *) data)[src * 2] * intVolume;
            //	s_rawsamples [dst].right = ((signed char *) data)[src * 2 + 1] * intVolume;
            s_rawsamples[dst].left =
                (((byte*) data)[src * 2] - 128) * intVolume;
            s_rawsamples[dst].right =
                (((byte*) data)[src * 2 + 1] - 128) * intVolume;
        }
    } else if (channels == 1 && width == 1) {
        intVolume *= 256;

        for (i = 0;; i++) {
            src = i * scale;
            if (src >= samples)
                break;
            dst = s_rawend & (MAX_RAW_SAMPLES - 1);
            s_rawend++;
            //	s_rawsamples [dst].left = ((signed char *) data)[src] * intVolume;
            //	s_rawsamples [dst].right = ((signed char *) data)[src] * intVolume;
            s_rawsamples[dst].left = (((byte*) data)[src] - 128) * intVolume;
            s_rawsamples[dst].right = (((byte*) data)[src] - 128) * intVolume;
        }
    }
}

/*
============
S_Update

Called once each time through the main loop
============
*/
void S_Update(vec3_t origin, vec3_t forward, vec3_t right, vec3_t up) {
    int i, j;
    int total;
    channel_t* ch;
    channel_t* combine;

    if (!sound_started || (snd_blocked > 0)) {
        return;
    }

    if (sfxvolume.value != old_sfxvolume) {
        SND_UpdateSfxVolume();
    }
    if (snd_filterquality.value != old_filterquality) {
        SND_UpdateFilterQuality();
    }

    VectorCopy(origin, listener_origin);
    VectorCopy(forward, listener_forward);
    VectorCopy(right, listener_right);
    VectorCopy(up, listener_up);

    // update general area ambient sound sources
    S_UpdateAmbientSounds();

    combine = NULL;

    // update spatialization for static and dynamic sounds
    ch = snd_channels + NUM_AMBIENTS;
    for (i = NUM_AMBIENTS; i < total_channels; i++, ch++) {
        if (!ch->sfx)
            continue;
        SND_Spatialize(ch); // respatialize channel
        if (!ch->leftvol && !ch->rightvol)
            continue;

        // try to combine static sounds with a previous channel of the same
        // sound effect so we don't mix five torches every frame

        if (i >= MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS) {
            // see if it can just use the last one
            if (combine && combine->sfx == ch->sfx) {
                combine->leftvol += ch->leftvol;
                combine->rightvol += ch->rightvol;
                ch->leftvol = ch->rightvol = 0;
                continue;
            }
            // search for one
            combine = snd_channels + MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS;
            for (j = MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS; j < i; j++, combine++) {
                if (combine->sfx == ch->sfx)
                    break;
            }

            if (j == total_channels) {
                combine = NULL;
            } else {
                if (combine != ch) {
                    combine->leftvol += ch->leftvol;
                    combine->rightvol += ch->rightvol;
                    ch->leftvol = ch->rightvol = 0;
                }
                continue;
            }
        }
    }

    //
    // debugging output
    //
    if (snd_show.value) {
        total = 0;
        ch = snd_channels;
        for (i = 0; i < total_channels; i++, ch++) {
            if (ch->sfx && (ch->leftvol || ch->rightvol)) {
                //Con_Printf("%3i %3i %s\n", ch->leftvol, ch->rightvol,
                //           ch->sfx->name);
                total++;
            }
        }

        Con_Printf("----(%i)----\n", total);
    }

    // mix some sound
    S_Update_();
}

static void GetSoundtime(void) {
    static int buffers;
    static int oldsamplepos;

    int fullsamples = shm->samples / shm->channels;

    // It is possible to miscount buffers if it has
    // wrapped twice between calls to S_Update.
    int samplepos = SNDDMA_GetDMAPos();

    if (samplepos < oldsamplepos) {
        // Buffer wrapped.
        buffers++;
        if (paintedtime > 0x40000000) {
            // Time to chop things off to avoid 32 bit limits.
            buffers = 0;
            paintedtime = fullsamples;
            S_StopAllSounds(true);
        }
    }
    oldsamplepos = samplepos;

    soundtime = buffers * fullsamples + samplepos / shm->channels;
}

void S_ExtraUpdate(void) {
    if (snd_noextraupdate.value != 0) {
        // don't pollute timings
        return;
    }
    S_Update_();
}

static void S_Update_(void) {
    unsigned endtime;
    int samps;

    if (!sound_started || (snd_blocked > 0)) {
        return;
    }

    SNDDMA_LockBuffer();
    if (!shm->buffer) {
        return;
    }

    // Updates DMA time.
    GetSoundtime();

    // Check to make sure that we haven't overshot.
    if (paintedtime < soundtime) {
        //Con_Printf("S_Update_ : overflow\n");
        paintedtime = soundtime;
    }

    // mix ahead of current position
    endtime = soundtime + _snd_mixahead.value * shm->speed;
    samps = shm->samples >> (shm->channels - 1);
    if (endtime - soundtime > samps)
        endtime = soundtime + samps;

    S_PaintChannels((int) endtime);

    SNDDMA_Submit();
}

/*
===============================================================================

CONSOLE FUNCTIONS

===============================================================================
*/

static void S_Play(void) {
    static int hash = 345;
    int i;
    char name[256];
    sfx_t* sfx;

    i = 1;
    while (i < Cmd_Argc()) {
        if (!Q_strrchr(Cmd_Argv(i), '.')) {
            Q_strcpy(name, Cmd_Argv(i));
            Q_strcat(name, ".wav");
        } else
            Q_strcpy(name, Cmd_Argv(i));
        sfx = S_PrecacheSound(name);
        S_StartSound(hash++, 0, sfx, listener_origin, 1.0f, 1.0f);
        i++;
    }
}

static void S_PlayVol(void) {
    static int hash = 543;
    int i;
    float vol;
    char name[256];
    sfx_t* sfx;

    i = 1;
    while (i < Cmd_Argc()) {
        if (!Q_strrchr(Cmd_Argv(i), '.')) {
            Q_strcpy(name, Cmd_Argv(i));
            Q_strcat(name, ".wav");
        } else
            Q_strcpy(name, Cmd_Argv(i));
        sfx = S_PrecacheSound(name);
        vol = Q_atof(Cmd_Argv(i + 1));
        S_StartSound(hash++, 0, sfx, listener_origin, vol, 1.0f);
        i += 2;
    }
}

static void S_SoundList(void) {
    int i;
    sfx_t* sfx;

    int total = 0;
    for (sfx = known_sfx, i = 0; i < num_sfx; i++, sfx++) {
        const sfxcache_t* sc = (sfxcache_t*) Cache_Check(&sfx->cache);
        if (!sc) {
            continue;
        }

        int size = sc->length * sc->width * (sc->stereo + 1);
        total += size;
        if (sc->loopstart >= 0) {
            Con_SafePrintf("L");
        } else {
            Con_SafePrintf(" ");
        }
        Con_SafePrintf("(%2db) %6i : %s\n", sc->width * 8, size, sfx->name);
    }
    Con_Printf("%i sounds, %i bytes\n", num_sfx, total);
}


void S_LocalSound(char* name) {
    if (nosound.value != 0) {
        return;
    }
    if (!sound_started) {
        return;
    }

    sfx_t* sfx = S_PrecacheSound(name);
    if (!sfx) {
        Con_Printf("S_LocalSound: can't cache %s\n", name);
        return;
    }
    S_StartSound(cl.viewentity, -1, sfx, vec3_origin, 1, 1);
}


void S_ClearPrecache(void) {
}


void S_BeginPrecaching(void) {
}


void S_EndPrecaching(void) {
}
