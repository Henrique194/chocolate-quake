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
// com_fs.c -- Quake filesystem
//
// All of Quake's data access is through a hierarchical file system,
// but the contents of the file system can be transparently merged
// from several sources.
//
// The "base directory" is the path to the directory holding the quake.exe
// and all game directories. The sys_* files pass this to host_init
// in quakeparms_t->basedir. This can be overridden with the "-basedir"
// command line parm to allow code debugging in a different directory.
// The base directory is only used during filesystem initialization.
//
// The "game directory" is the first tree on the search path and directory
// that all generated files (savegames, screenshots, demos, config files)
// will be saved to. This can be overridden with the "-game" command line
// parameter. The game directory can never be changed while quake is
// executing. This is a precaution against having a malicious server
// instruct clients to write files over areas they shouldn't.
//
// The "cache directory" is only used during development to save network
// bandwidth, especially over ISDN / T1 lines. If there is a cache
// directory specified, when a file is found by the normal search path,
// it will be mirrored into the cache directory, then opened there.
//


#include "quakedef.h"
#include "console.h"
#include "crc.h"
#include "cvar.h"
#include "draw.h"
#include "net.h"
#include "sys.h"
#include "zone.h"
#include <string.h>


// if a packfile directory differs from this, it is assumed to be hacked
#define PAK0_COUNT 339
#define PAK0_CRC   32981

#define MAX_FILES_IN_PACK 2048


//
// in memory
//

typedef struct {
    char name[MAX_QPATH];
    i32 filepos;
    i32 filelen;
} packfile_t;

typedef struct {
    char filename[MAX_OSPATH];
    i32 handle;
    i32 numfiles;
    packfile_t* files;
} pack_t;

//
// on disk
//

typedef struct {
    char name[56];
    i32 filepos;
    i32 filelen;
} dpackfile_t;

typedef struct {
    char id[4];
    i32 dirofs;
    i32 dirlen;
} dpackheader_t;


typedef struct searchpath_s {
    char filename[MAX_OSPATH];
    // Only one of filename/pack will be used.
    pack_t* pack;
    struct searchpath_s* next;
} searchpath_t;


qboolean com_modified;

static qboolean proghack;

qboolean msg_suppress_1 = 0;

qboolean standard_quake = true;
qboolean rogue;
qboolean hipnotic;

i32 com_filesize;

static char com_cachedir[MAX_OSPATH];
char com_gamedir[MAX_OSPATH];

static searchpath_t* com_searchpaths;

static cache_user_t* loadcache;
static byte* loadbuf;
static i32 loadsize;


/*
============
COM_Path_f

============
*/
void COM_Path_f(void) {
    searchpath_t* s = com_searchpaths;
    Con_Printf("Current search path:\n");
    for (; s; s = s->next) {
        if (s->pack) {
            Con_Printf("%s (%i files)\n", s->pack->filename, s->pack->numfiles);
        } else {
            Con_Printf("%s\n", s->filename);
        }
    }
}


/*
============
COM_WriteFile

The filename will be prefixed by the current game directory
============
*/
void COM_WriteFile(char* filename, void* data, i32 len) {
    char name[MAX_OSPATH];
    sprintf(name, "%s/%s", com_gamedir, filename);

    i32 handle = Sys_FileOpenWrite(name);
    if (handle == -1) {
        Sys_Printf("COM_WriteFile: failed on %s\n", name);
        return;
    }

    Sys_Printf("COM_WriteFile: %s\n", name);
    Sys_FileWrite(handle, data, len);
    Sys_FileClose(handle);
}


/*
================================================================================

FILE OPENING/CLOSING

================================================================================
*/

/*
============
COM_CreatePath

Only used for CopyFile
============
*/
static void COM_CreatePath(char* path) {
    for (char* ofs = path + 1; *ofs; ofs++) {
        if (*ofs == '/') {
            // Create the directory.
            *ofs = 0;
            Sys_mkdir(path);
            *ofs = '/';
        }
    }
}

/*
===========
COM_CopyFile

Copies a file over from the net to the local cache,
creating any directories needed. This is for the
convenience of developers using ISDN from home.
===========
*/
static void COM_CopyFile(char* netpath, char* cachepath) {
    char buf[4096];

    i32 in;
    i32 remaining = Sys_FileOpenRead(netpath, &in);
    COM_CreatePath(cachepath); // create directories up to the cache file
    i32 out = Sys_FileOpenWrite(cachepath);

    while (remaining) {
        i32 count = SDL_min(remaining, sizeof(buf));
        Sys_FileRead(in, buf, count);
        Sys_FileWrite(out, buf, count);
        remaining -= count;
    }

    Sys_FileClose(in);
    Sys_FileClose(out);
}

/*
============
COM_SearchDir

Check a file in the directory tree.
============
*/
static qboolean COM_SearchDir(
    const searchpath_t* search,
    const char* filename,
    i32* handle,
    FILE** file
) {
    char netpath[MAX_OSPATH];
    char cachepath[MAX_OSPATH];
    i32 i;

    sprintf(netpath, "%s/%s", search->filename, filename);
    i32 findtime = Sys_FileTime(netpath);
    if (findtime == -1) {
        return false;
    }

    // see if the file needs to be updated in the cache
    if (com_cachedir[0]) {
        sprintf(cachepath, "%s%s", com_cachedir, netpath);
        i32 cachetime = Sys_FileTime(cachepath);
        if (cachetime < findtime) {
            COM_CopyFile(netpath, cachepath);
        }
        Q_strcpy(netpath, cachepath);
    } else {
        Q_strcpy(cachepath, netpath);
    }

    Sys_Printf("FindFile: %s\n", netpath);
    com_filesize = Sys_FileOpenRead(netpath, &i);
    if (handle) {
        *handle = i;
    } else {
        Sys_FileClose(i);
        *file = fopen(netpath, "rb");
    }
    return true;
}

/*
============
COM_SearchPak

Look through all the pak file elements.
============
*/
static qboolean COM_SearchPak(
    const pack_t* pak,
    const char* filename,
    i32* handle,
    FILE** file
) {
    for (i32 i = 0; i < pak->numfiles; i++) {
        if (Q_strcmp(pak->files[i].name, filename) != 0) {
            continue;
        }
        // found it!
        Sys_Printf("PackFile: %s : %s\n", pak->filename, filename);
        if (handle) {
            *handle = pak->handle;
            Sys_FileSeek(pak->handle, pak->files[i].filepos);
        } else {
            // open a new file on the pakfile
            *file = fopen(pak->filename, "rb");
            if (*file) {
                fseek(*file, pak->files[i].filepos, SEEK_SET);
            }
        }
        com_filesize = pak->files[i].filelen;
        return true;
    }
    return false;
}

static qboolean COM_SearchPath(
    const searchpath_t* search,
    const char* filename,
    i32* handle,
    FILE** file
) {
    if (search->pack) {
        return COM_SearchPak(search->pack, filename, handle, file);
    }
    if (!static_registered && (Q_strchr(filename, '/') || Q_strchr(filename, '\\'))) {
        // if not a registered version, don't ever go beyond base
        return false;
    }
    return COM_SearchDir(search, filename, handle, file);
}


/*
============
COM_SearchPaths

Search through the path, one element at a time.
============
*/
static qboolean COM_SearchPaths(
    const char* filename,
    i32* handle,
    FILE** file
) {
    const searchpath_t* search = com_searchpaths;
    if (proghack && !Q_strcmp(filename, "progs.dat")) {
        // gross hack to use quake 1 progs with quake 2 maps
        search = search->next;
    }
    for (; search; search = search->next) {
        if (COM_SearchPath(search, filename, handle, file)) {
            return true;
        }
    }
    return false;
}

/*
===========
COM_FindFile

Finds the file in the search path.
Sets com_filesize and one of handle or file
===========
*/
static i32 COM_FindFile(char* filename, i32* handle, FILE** file) {
    if (file && handle) {
        Sys_Error("COM_FindFile: both handle and file set");
    }
    if (!file && !handle) {
        Sys_Error("COM_FindFile: neither handle or file set");
    }
    if (!COM_SearchPaths(filename, handle, file)) {
        Sys_Printf("FindFile: can't find %s\n", filename);
        if (handle) {
            *handle = -1;
        } else {
            *file = NULL;
        }
        com_filesize = -1;
        return -1;
    }
    return com_filesize;
}

/*
===========
COM_OpenFile

filename never has a leading slash, but may contain directory walks
returns a handle and a length
it may actually be inside a pak file
===========
*/
i32 COM_OpenFile(char* filename, i32* handle) {
    return COM_FindFile(filename, handle, NULL);
}

/*
===========
COM_FOpenFile

If the requested file is inside a packfile, a new FILE * will be opened
into the file.
===========
*/
i32 COM_FOpenFile(char* filename, FILE** file) {
    return COM_FindFile(filename, NULL, file);
}

/*
============
COM_CloseFile

If it is a pak file handle, don't really close it
============
*/
void COM_CloseFile(i32 h) {
    searchpath_t* s = com_searchpaths;
    for (; s; s = s->next) {
        if (s->pack && s->pack->handle == h) {
            return;
        }
    }
    Sys_FileClose(h);
}

//==============================================================================


/*
================================================================================

MUSIC TRACK FILES

================================================================================
*/

i32 COM_FindMusicTrack(const char* track_file, FILE** file) {
    const searchpath_t* search = com_searchpaths;
    for (; search; search = search->next) {
        if (search->pack) {
            continue;
        }
        if (COM_SearchDir(search, track_file, NULL, file)) {
            return com_filesize;
        }
    }
    Sys_Printf("FindMusicTrack: can't find %s\n", track_file);
    *file = NULL;
    com_filesize = -1;
    return -1;
}

qboolean COM_MusicTrackExists(const char* track_file) {
    FILE* file = NULL;
    COM_FindMusicTrack(track_file, &file);
    if (file) {
        fclose(file);
        return true;
    }
    return false;
}

//==============================================================================


/*
================================================================================

FILE READING

================================================================================
*/

/*
============
COM_LoadFile

Filename are relative to the quake directory.
Allways appends a 0 byte.
============
*/
static byte* COM_LoadFile(char* path, i32 usehunk) {
    // look for it in the filesystem or pack files
    i32 h;
    i32 len = COM_OpenFile(path, &h);
    if (h == -1) {
        return NULL;
    }

    // extract the filename base name for hunk tag
    char base[32];
    COM_FileBase(path, base, sizeof(base));
    byte* buf = NULL; // quiet compiler warning
    switch (usehunk) {
        case 1:
            buf = Hunk_AllocName(len + 1, base);
            break;
        case 2:
            buf = Hunk_TempAlloc(len + 1);
            break;
        case 3:
            buf = Cache_Alloc(loadcache, len + 1, base);
            break;
        case 4:
            if (len + 1 > loadsize) {
                buf = Hunk_TempAlloc(len + 1);
            } else {
                buf = loadbuf;
            }
            break;
        default:
            Sys_Error("COM_LoadFile: bad usehunk");
            break;
    }
    if (!buf) {
        Sys_Error("COM_LoadFile: not enough space for %s", path);
    }
    buf[len] = 0;

    Draw_BeginDisc();
    Sys_FileRead(h, buf, len);
    COM_CloseFile(h);
    Draw_EndDisc();

    return buf;
}

byte* COM_LoadHunkFile(char* path) {
    return COM_LoadFile(path, 1);
}

byte* COM_LoadTempFile(char* path) {
    return COM_LoadFile(path, 2);
}

void COM_LoadCacheFile(char* path, struct cache_user_s* cu) {
    loadcache = cu;
    COM_LoadFile(path, 3);
}

// uses temp hunk if larger than bufsize
byte* COM_LoadStackFile(char* path, void* buffer, i32 bufsize) {
    loadbuf = (byte*) buffer;
    loadsize = bufsize;
    byte* buf = COM_LoadFile(path, 4);
    return buf;
}

/*
=================
COM_LoadPackFile

Takes an explicit (not game tree related) path to a pak file.

Loads the header and directory, adding the files at the beginning
of the list so they override previous pack files.
=================
*/
static pack_t* COM_LoadPackFile(char* packfile) {
    dpackheader_t header;
    dpackfile_t info[MAX_FILES_IN_PACK];
    i32 packhandle;

    if (Sys_FileOpenRead(packfile, &packhandle) == -1) {
        // Con_Printf("Couldn't open %s\n", packfile);
        return NULL;
    }
    Sys_FileRead(packhandle, (void*) &header, sizeof(header));
    if (Q_strncmp(header.id, "PACK", 4)) {
        Sys_Error("%s is not a packfile", packfile);
    }
    header.dirofs = LittleLong(header.dirofs);
    header.dirlen = LittleLong(header.dirlen);

    i32 numpackfiles = header.dirlen / sizeof(dpackfile_t);
    if (numpackfiles > MAX_FILES_IN_PACK) {
        Sys_Error("%s has %i files", packfile, numpackfiles);
    }
    if (numpackfiles != PAK0_COUNT) {
        // not the original file
        com_modified = true;
    }
    packfile_t* newfiles = Hunk_AllocName(numpackfiles * sizeof(*newfiles), "packfile");

    Sys_FileSeek(packhandle, header.dirofs);
    Sys_FileRead(packhandle, (void*) info, header.dirlen);

    // crc the directory to check for modifications
    u16 crc;
    CRC_Init(&crc);
    for (i32 i = 0; i < header.dirlen; i++) {
        CRC_ProcessByte(&crc, ((byte*) info)[i]);
    }
    if (crc != PAK0_CRC) {
        com_modified = true;
    }

    // parse the directory
    for (i32 i = 0; i < numpackfiles; i++) {
        Q_strcpy(newfiles[i].name, info[i].name);
        newfiles[i].filepos = LittleLong(info[i].filepos);
        newfiles[i].filelen = LittleLong(info[i].filelen);
    }

    pack_t* pack = Hunk_Alloc(sizeof(*pack));
    Q_strcpy(pack->filename, packfile);
    pack->handle = packhandle;
    pack->numfiles = numpackfiles;
    pack->files = newfiles;

    Con_Printf("Added packfile %s (%i files)\n", packfile, numpackfiles);
    return pack;
}

//==============================================================================


/*
================================================================================

INITIALIZATION

================================================================================
*/

static char* COM_GetBaseDir(void) {
    static char basedir[MAX_OSPATH] = {0};
    if (basedir[0]) {
        return basedir;
    }
    //
    // -basedir <path>
    // Overrides the system supplied base directory (under GAMENAME)
    //
    i32 i = COM_CheckParm("-basedir");
    if (i && i < com_argc - 1) {
        Q_strcpy(basedir, com_argv[i + 1]);
    } else {
        Q_strcpy(basedir, host_parms.basedir);
    }
    size_t j = Q_strlen(basedir);
    if (j > 0 && ((basedir[j - 1] == '\\') || (basedir[j - 1] == '/'))) {
        basedir[j - 1] = 0;
    }
    return basedir;
}

/*
================
COM_AddGameDirectory

Sets com_gamedir, adds the directory to the head of the path,
then loads and adds pak1.pak, pak2.pak, ...
================
*/
static void COM_AddGameDirectory(char* dir) {
    char pakfile[MAX_OSPATH];

    Q_strcpy(com_gamedir, dir);

    //
    // Add the directory to the search path.
    //
    searchpath_t* search = Hunk_Alloc(sizeof(*search));
    Q_strcpy(search->filename, dir);
    search->next = com_searchpaths;
    com_searchpaths = search;

    //
    // Add any pak files in the format pak0.pak, pak1.pak, ...
    //
    for (i32 i = 0;; i++) {
        sprintf(pakfile, "%s/pak%i.pak", dir, i);
        pack_t* pak = COM_LoadPackFile(pakfile);
        if (!pak) {
            break;
        }
        search = Hunk_Alloc(sizeof(*search));
        search->pack = pak;
        search->next = com_searchpaths;
        com_searchpaths = search;
    }
}

static void COM_AddGameDirs(void) {
    char* basedir = COM_GetBaseDir();

    //
    // start up with GAMENAME by default (id1)
    //
    COM_AddGameDirectory(va("%s/" GAMENAME, basedir));
    if (COM_CheckParm("-rogue")) {
        COM_AddGameDirectory(va("%s/rogue", basedir));
    }
    if (COM_CheckParm("-hipnotic")) {
        COM_AddGameDirectory(va("%s/hipnotic", basedir));
    }

    //
    // -game <gamedir>
    // Adds basedir/gamedir as an override game
    //
    i32 i = COM_CheckParm("-game");
    if (i && i < com_argc - 1) {
        com_modified = true;
        COM_AddGameDirectory(va("%s/%s", basedir, com_argv[i + 1]));
    }
}

static void COM_SetCacheDir(void) {
    //
    // -cachedir <path>
    // Overrides the system supplied cache directory (NULL or /qcache)
    // -cachedir - will disable caching.
    //
    i32 i = COM_CheckParm("-cachedir");
    if (i && i < com_argc - 1) {
        if (com_argv[i + 1][0] == '-') {
            com_cachedir[0] = 0;
        } else {
            Q_strcpy(com_cachedir, com_argv[i + 1]);
        }
        return;
    }
    if (host_parms.cachedir) {
        Q_strcpy(com_cachedir, host_parms.cachedir);
        return;
    }
    com_cachedir[0] = 0;
}

static void COM_UseCustomPaths(i32 arg_num) {
    com_modified = true;
    com_searchpaths = NULL;
    for (i32 i = arg_num; i < com_argc; i++) {
        char* arg = com_argv[i];
        if (!arg || *arg == '+' || *arg == '-') {
            break;
        }
        searchpath_t* search = Hunk_Alloc(sizeof(*search));
        const char* ext = COM_FileExtension(arg);
        if (!Q_strcmp(ext, "pak")) {
            search->pack = COM_LoadPackFile(arg);
            if (!search->pack) {
                Sys_Error("Couldn't load packfile: %s", arg);
            }
        } else {
            Q_strcpy(search->filename, arg);
        }
        search->next = com_searchpaths;
        com_searchpaths = search;
    }
}

/*
================
COM_InitFilesystem
================
*/
void COM_InitFilesystem(void) {
    COM_AddGameDirs();
    COM_SetCacheDir();

    //
    // -path <dir or packfile> [<dir or packfile>] ...
    // Fully specifies the exact search path, overriding the generated one
    //
    i32 i = COM_CheckParm("-path");
    if (i) {
        COM_UseCustomPaths(i + 1);
    }

    if (COM_CheckParm("-proghack")) {
        proghack = true;
    }
}

//==============================================================================
