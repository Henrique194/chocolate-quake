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


#include "quakedef.h"
#include "snd_codec.h"


#define MUSIC_DIRNAME "music"

typedef enum _bgm_player {
    BGM_NONE = -1,
    BGM_MIDIDRV = 1,
    BGM_STREAMER
} bgm_player_t;

typedef struct music_handler_s {
    unsigned int type;   /* 1U << n (see snd_codec.h)	*/
    bgm_player_t player; /* Enumerated bgm player type	*/
    int is_available;    /* -1 means not present		*/
    const char* ext;     /* Expected file extension	*/
    const char* dir;     /* Where to look for music file */
    struct music_handler_s* next;
} music_handler_t;

static music_handler_t* music_handlers = NULL;
static snd_stream_t* bgmstream = NULL;
static qboolean no_extmusic = false;

static music_handler_t wanted_handlers[] = {
    {CODECTYPE_VORBIS, BGM_STREAMER, -1, "ogg", MUSIC_DIRNAME, NULL},
    {CODECTYPE_OPUS, BGM_STREAMER, -1, "opus", MUSIC_DIRNAME, NULL},
    {CODECTYPE_MP3, BGM_STREAMER, -1, "mp3", MUSIC_DIRNAME, NULL},
    {CODECTYPE_FLAC, BGM_STREAMER, -1, "flac", MUSIC_DIRNAME, NULL},
    {CODECTYPE_WAV, BGM_STREAMER, -1, "wav", MUSIC_DIRNAME, NULL},
    {CODECTYPE_MOD, BGM_STREAMER, -1, "it", MUSIC_DIRNAME, NULL},
    {CODECTYPE_MOD, BGM_STREAMER, -1, "s3m", MUSIC_DIRNAME, NULL},
    {CODECTYPE_MOD, BGM_STREAMER, -1, "xm", MUSIC_DIRNAME, NULL},
    {CODECTYPE_MOD, BGM_STREAMER, -1, "mod", MUSIC_DIRNAME, NULL},
    {CODECTYPE_UMX, BGM_STREAMER, -1, "umx", MUSIC_DIRNAME, NULL},
    {CODECTYPE_NONE, BGM_NONE, -1, NULL, NULL, NULL}};

#define ANY_CODECTYPE 0xFFFFFFFF
#define CDRIP_TYPES                                                            \
    (CODECTYPE_VORBIS | CODECTYPE_MP3 | CODECTYPE_FLAC | CODECTYPE_WAV |       \
     CODECTYPE_OPUS)
#define CDRIPTYPE(x) (((x) & CDRIP_TYPES) != 0)


static qboolean playLooping = false;
static qboolean initialized = false;
static qboolean enabled = true;
static qboolean playing = false;
static qboolean wasPlaying = false;
static byte playTrack;
static float cdvolume;
static byte remap[100];


static void CD_f(void) {
    char* command;
    int ret;
    int n;

    if (Cmd_Argc() < 2) {
        return;
    }

    command = Cmd_Argv(1);

    if (Q_strcasecmp(command, "on") == 0) {
        enabled = true;
        return;
    }

    if (Q_strcasecmp(command, "off") == 0) {
        if (playing) {
            BGMusic_Stop();
        }
        enabled = false;
        return;
    }

    if (Q_strcasecmp(command, "reset") == 0) {
        enabled = true;
        if (playing) {
            BGMusic_Stop();
        }
        for (n = 0; n < 100; n++) {
            remap[n] = (byte) n;
        }
        return;
    }

    if (Q_strcasecmp(command, "remap") == 0) {
        ret = Cmd_Argc() - 2;
        if (ret <= 0) {
            for (n = 1; n < 100; n++) {
                if (remap[n] != n) {
                    Con_Printf("  %u -> %u\n", n, remap[n]);
                }
            }
            return;
        }
        for (n = 1; n <= ret; n++) {
            remap[n] = (byte) Q_atoi(Cmd_Argv(n + 1));
        }
        return;
    }

    if (Q_strcasecmp(command, "play") == 0) {
        BGMusic_Play((byte) Q_atoi(Cmd_Argv(2)), false);
        return;
    }
    if (Q_strcasecmp(command, "loop") == 0) {
        BGMusic_Play((byte) Q_atoi(Cmd_Argv(2)), true);
        return;
    }

    if (Q_strcasecmp(command, "stop") == 0) {
        BGMusic_Stop();
        return;
    }

    if (Q_strcasecmp(command, "pause") == 0) {
        BGMusic_Pause();
        return;
    }

    if (Q_strcasecmp(command, "resume") == 0) {
        BGMusic_Resume();
        return;
    }

    if (Q_strcasecmp(command, "info") == 0) {
        //        Con_Printf("%u tracks\n", maxTrack);
        if (playing) {
            Con_Printf("Currently %s track %u\n",
                       playLooping ? "looping" : "playing", playTrack);
        } else if (wasPlaying) {
            Con_Printf("Paused %s track %u\n",
                       playLooping ? "looping" : "playing", playTrack);
        }
        Con_Printf("Volume is %f\n", cdvolume);
        return;
    }
}


static void BGMusic_GetTrackPath(char* tmp, byte track, const char* ext) {
    snprintf(tmp, MAX_QPATH, "%s/track%02d.%s", MUSIC_DIRNAME, (int) track,
             ext);
}

void BGMusic_Play(byte track, qboolean looping) {
    char tmp[MAX_QPATH];

    if (music_handlers == NULL) {
        return;
    }
    if (no_extmusic) {
        return;
    }

    track = remap[track];
    if (track < 1) {
        Con_DPrintf("CDAudio: Bad track number %u.\n", track);
        return;
    }

    if (playing) {
        if (playTrack == track) {
            return;
        }
        BGMusic_Stop();
    }

    unsigned int type;
    const char* ext = NULL;
    music_handler_t* handler = music_handlers;

    for (; handler; handler = handler->next) {
        if (!handler->is_available) {
            continue;
        }
        if (!CDRIPTYPE(handler->type)) {
            continue;
        }
        BGMusic_GetTrackPath(tmp, track, handler->ext);
        if (COM_FileExists(tmp)) {
            type = handler->type;
            ext = handler->ext;
            break;
        }
    }
    if (ext == NULL) {
        Con_Printf("Couldn't find a cdrip for track %d\n", (int) track);
        return;
    }

    BGMusic_GetTrackPath(tmp, track, ext);
    bgmstream = S_CodecOpenStreamType(tmp, type, playLooping);
    if (!bgmstream) {
        Con_Printf("Couldn't handle music file %s\n", tmp);
    }

    playLooping = looping;
    playTrack = track;
    playing = true;

    if (cdvolume == 0.0) {
        BGMusic_Pause();
    }
}


void BGMusic_Stop() {
    if (!bgmstream || !enabled) {
        return;
    }
    if (!playing) {
        return;
    }
    bgmstream->status = STREAM_NONE;
    S_CodecCloseStream(bgmstream);
    bgmstream = NULL;
    s_rawend = 0;
    playing = false;
    wasPlaying = false;
}


void BGMusic_Pause() {
    if (!bgmstream || !enabled) {
        return;
    }
    if (!playing || bgmstream->status != STREAM_PLAY) {
        return;
    }
    bgmstream->status = STREAM_PAUSE;
    wasPlaying = playing;
    playing = false;
}


void BGMusic_Resume() {
    if (!bgmstream || !enabled) {
        return;
    }
    if (!wasPlaying || bgmstream->status != STREAM_PAUSE) {
        return;
    }
    bgmstream->status = STREAM_PLAY;
    playing = true;
}


int BGMusic_Init() {
    if (cls.state == ca_dedicated) {
        return -1;
    }
    if (COM_CheckParm("-nocdaudio")) {
        no_extmusic = true;
        return -1;
    }

    // WRITE CODE TO CHANGE MUSIC DIR
    //    if ((i = COM_CheckParm("-cddev")) != 0 && i < com_argc - 1) {
    //        strncpy(cd_dev, com_argv[i + 1], sizeof(cd_dev));
    //        cd_dev[sizeof(cd_dev) - 1] = 0;
    //    }

    // WRITE CODE TO OPEN MUSIC FILES
    //    if ((cdfile = open(cd_dev, O_RDONLY)) == -1) {
    //        Con_Printf("BGMusic_Init: open of \"%s\" failed (%i)\n", cd_dev, errno);
    //        cdfile = -1;
    //        return -1;
    //    }

    for (byte i = 0; i < 100; i++) {
        remap[i] = i;
    }
    initialized = true;
    enabled = true;

    Cmd_AddCommand("cd", CD_f);
    Con_Printf("CD Audio Initialized\n");


    music_handler_t* handlers = NULL;
    playLooping = true;

    for (int i = 0; wanted_handlers[i].type != CODECTYPE_NONE; i++) {
        switch (wanted_handlers[i].player) {
            case BGM_MIDIDRV:
                /* not supported in quake */
                break;
            case BGM_STREAMER:
                wanted_handlers[i].is_available =
                    S_CodecIsAvailable(wanted_handlers[i].type);
                break;
            case BGM_NONE:
            default:
                break;
        }

        if (wanted_handlers[i].is_available != -1) {
            if (handlers) {
                handlers->next = &wanted_handlers[i];
                handlers = handlers->next;
            } else {
                music_handlers = &wanted_handlers[i];
                handlers = music_handlers;
            }
        }
    }

    return 0;
}


void BGMusic_Shutdown() {
    if (!initialized) {
        return;
    }
    BGMusic_Stop();
    // Sever our connections to midi_drv and snd_codec.
    music_handlers = NULL;
}


static qboolean did_rewind = false;
static byte raw_audio_buffer[16384];

static int BGMusic_EndOfFile() {
    if (!playLooping) {
        return 0;
    }
    // Try to loop music.
    if (did_rewind) {
        Con_Printf("Stream keeps returning EOF.\n");
        return -1;
    }
    int bytes_read = S_CodecRewindStream(bgmstream);
    if (bytes_read != 0) {
        Con_Printf("Stream seek error (%i), stopping.\n", bytes_read);
        return -1;
    }
    did_rewind = true;
    return bytes_read;
}

static int BGMusic_ReadStream(int file_samples, int file_size) {
    const snd_info_t* info = &bgmstream->info;

    int bytes_read = S_CodecReadStream(bgmstream, file_size, raw_audio_buffer);
    if (bytes_read < file_size) {
        file_samples = bytes_read / (info->width * info->channels);
    }

    if (bytes_read > 0) {
        // Data: add to raw buffer.
        S_RawSamples(file_samples, info->rate, info->width, info->channels,
                     raw_audio_buffer, bgmvolume.value);
        did_rewind = false;
        return bytes_read;
    }
    if (bytes_read == 0) {
        return BGMusic_EndOfFile();
    }
    // Some read error.
    Con_Printf("Stream read error (%i), stopping.\n", bytes_read);
    return -1;
}

static void BGMusic_GetStreamInfo(int* file_samples, int* file_size) {
    const snd_info_t* info = &bgmstream->info;

    // Decide how much data needs to be read from the file.
    int buffer_samples = MAX_RAW_SAMPLES - (s_rawend - paintedtime);
    *file_samples = buffer_samples * info->rate / shm->speed;
    if (*file_samples == 0) {
        return;
    }

    // Our max buffer size.
    int file_sample_size = info->width * info->channels;
    *file_size = (*file_samples) * file_sample_size;
    if (*file_size > (int) sizeof(raw_audio_buffer)) {
        *file_size = (int) sizeof(raw_audio_buffer);
        *file_samples = (*file_size) / file_sample_size;
    }
}

static void BGMusic_UpdateStream() {
    if (bgmstream->status != STREAM_PLAY) {
        return;
    }
    if (bgmvolume.value <= 0) {
        // Don't bother playing anything if musicvolume is 0.
        return;
    }

    did_rewind = false;
    if (s_rawend < paintedtime) {
        // See how many samples should be copied into the raw buffer.
        s_rawend = paintedtime;
    }
    while (s_rawend < paintedtime + MAX_RAW_SAMPLES) {
        int file_samples;
        int file_size;
        BGMusic_GetStreamInfo(&file_samples, &file_size);
        if (!file_samples || !file_size) {
            return;
        }
        int bytes_read = BGMusic_ReadStream(file_samples, file_size);
        if (bytes_read <= 0) {
            BGMusic_Stop();
            return;
        }
    }
}

void BGMusic_Update() {
    if (!enabled) {
        return;
    }

    if (bgmvolume.value != cdvolume) {
        if (cdvolume != 0) {
            Cvar_SetValue("bgmvolume", 0.0f);
            cdvolume = bgmvolume.value;
            BGMusic_Pause();
        } else {
            Cvar_SetValue("bgmvolume", 1.0f);
            cdvolume = bgmvolume.value;
            BGMusic_Resume();
        }
    }
    if (bgmstream) {
        BGMusic_UpdateStream();
    }
}
