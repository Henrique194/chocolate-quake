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
// common.h  -- general definitions


#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdio.h>
#include <stdint.h>


#ifndef NULL
#define NULL ((void*) 0)
#endif

#define Q_MAXCHAR  ((char) 0x7f)
#define Q_MAXSHORT ((short) 0x7fff)
#define Q_MAXINT   ((int) 0x7fffffff)
#define Q_MAXLONG  ((int) 0x7fffffff)
#define Q_MAXFLOAT ((int) 0x7fffffff)

#define Q_MINCHAR  ((char) 0x80)
#define Q_MINSHORT ((short) 0x8000)
#define Q_MININT   ((int) 0x80000000)
#define Q_MINLONG  ((int) 0x80000000)
#define Q_MINFLOAT ((int) 0x7fffffff)


#if !defined BYTE_DEFINED
typedef unsigned char byte;
#define BYTE_DEFINED 1
#endif

#undef true
#undef false

typedef enum {
    false,
    true,
} qboolean;


/*
================================================================================

COMMAND LINE

================================================================================
*/

#define CMDLINE_LENGTH 256

extern int com_argc;
extern char** com_argv;
extern char com_cmdline[CMDLINE_LENGTH];

void COM_InitArgv(int argc, char** argv);
int COM_CheckParm(const char* parm);

//==============================================================================


/*
================================================================================

BYTE ORDER FUNCTIONS

================================================================================
*/

extern qboolean bigendien;

extern short (*BigShort)(short l);
extern short (*LittleShort)(short l);
extern int (*BigLong)(int l);
extern int (*LittleLong)(int l);
extern float (*BigFloat)(float l);
extern float (*LittleFloat)(float l);

//==============================================================================


/*
================================================================================

FILE EXTENSION

================================================================================
*/

void COM_StripExtension(const char* in, char* out);
char* COM_FileExtension(const char* in);
void COM_FileBase(const char* in, char* out, size_t outsize);
void COM_DefaultExtension(char* path, const char* extension);

//==============================================================================


/*
================================================================================

QUAKE FILESYSTEM

================================================================================
*/

struct cache_user_s;

extern qboolean com_modified; // set true if using non-id files

extern int com_filesize;
extern char com_gamedir[MAX_OSPATH];

extern qboolean standard_quake;
extern qboolean rogue;
extern qboolean hipnotic;

void COM_Path_f(void);
void COM_WriteFile(char* filename, void* data, int len);

int COM_OpenFile(char* filename, int* hndl);
int COM_FOpenFile(char* filename, FILE** file);
void COM_CloseFile(int h);

int COM_FindMusicTrack(const char* track_file, FILE** file);
qboolean COM_MusicTrackExists(const char* track_file);

byte* COM_LoadStackFile(char* path, void* buffer, int bufsize);
byte* COM_LoadTempFile(char* path);
byte* COM_LoadHunkFile(char* path);
void COM_LoadCacheFile(char* path, struct cache_user_s* cu);

void COM_InitFilesystem(void);

//==============================================================================


/*
================================================================================

COMMON INITIALIZATION

================================================================================
*/

extern struct cvar_s registered;
// only for startup check, then set
extern int static_registered;

void COM_Init(char* path);

//==============================================================================


/*
================================================================================

DOUBLE LINKED LIST

================================================================================
*/

// (type *)STRUCT_FROM_LINK(link_t *link, type, member)
// ent = STRUCT_FROM_LINK(link,entity_t,order)
// FIXME: remove this mess!
#define STRUCT_FROM_LINK(l, t, m) ((t*) ((byte*) l - (intptr_t) &(((t*) 0)->m)))

typedef struct link_s {
    struct link_s* prev;
    struct link_s* next;
} link_t;

void ClearLink(link_t* l);
void RemoveLink(link_t* l);
void InsertLinkBefore(link_t* l, link_t* before);
void InsertLinkAfter(link_t* l, link_t* after);

//==============================================================================


/*
================================================================================

SIZEBUF

================================================================================
*/

typedef struct {
    qboolean allowoverflow; // if false, do a Sys_Error
    qboolean overflowed;    // set to true if the buffer size failed
    byte* data;
    int maxsize;
    int cursize;
} sizebuf_t;

void SZ_Alloc(sizebuf_t* buf, int startsize);
void SZ_Free(sizebuf_t* buf);
void SZ_Clear(sizebuf_t* buf);
void* SZ_GetSpace(sizebuf_t* buf, int length);
void SZ_Write(sizebuf_t* buf, void* data, int length);
void SZ_Print(sizebuf_t* buf, char* data); // strcats onto the sizebuf

//=============================================================================


/*
================================================================================

MESSAGE IO FUNCTIONS

================================================================================
*/

extern int msg_readcount;
extern qboolean msg_badread; // set if a read goes beyond end of message

void MSG_WriteChar(sizebuf_t* sb, int c);
void MSG_WriteByte(sizebuf_t* sb, int c);
void MSG_WriteShort(sizebuf_t* sb, int c);
void MSG_WriteLong(sizebuf_t* sb, int c);
void MSG_WriteFloat(sizebuf_t* sb, float f);
void MSG_WriteString(sizebuf_t* sb, char* s);
void MSG_WriteCoord(sizebuf_t* sb, float f);
void MSG_WriteAngle(sizebuf_t* sb, float f);

void MSG_BeginReading(void);
int MSG_ReadChar(void);
int MSG_ReadByte(void);
int MSG_ReadShort(void);
int MSG_ReadLong(void);
float MSG_ReadFloat(void);
char* MSG_ReadString(void);
float MSG_ReadCoord(void);
float MSG_ReadAngle(void);

//==============================================================================


/*
================================================================================

STDIO REPLACEMENT FUNCTIONS

================================================================================
*/

typedef struct {
    FILE* file;
    long start;  // file or data start position
    long length; // file or data size
    long pos;    // current position relative to start
} fshandle_t;

size_t Q_fread(void* ptr, size_t size, size_t nmemb, fshandle_t* fh);
int Q_fseek(fshandle_t* fh, long offset, int whence);
long Q_ftell(const fshandle_t* fh);
void Q_rewind(fshandle_t* fh);
long Q_filelength(const fshandle_t* fh);
int Q_feof(const fshandle_t *fh);
int Q_ferror(fshandle_t* fh);

//==============================================================================


/*
================================================================================

STRING LIB REPLACEMENT FUNCTIONS

================================================================================
*/

void Q_memset(void* dest, int fill, int count);
void Q_memcpy(void* dest, void* src, int count);
int Q_memcmp(void* m1, void* m2, int count);
void Q_strcpy(char* dest, const char* src);
void Q_strncpy(char* dest, const char* src, int count);
int Q_strlen(const char* str);
char* Q_strrchr(char* s, char c);
void Q_strcat(char* dest, const char* src);
int Q_strcmp(const char* s1, const char* s2);
int Q_strncmp(const char* s1, const char* s2, int count);
int Q_strcasecmp(const char* s1, const char* s2);
int Q_strncasecmp(const char* s1, const char* s2, int n);
int Q_atoi(const char* str);
float Q_atof(const char* str);

//==============================================================================


/*
================================================================================

TOKEN PARSER

================================================================================
*/

extern char com_token[1024];

char* COM_Parse(char* data);

//==============================================================================


// does a varargs printf into a temp buffer
char* va(char* format, ...);

#endif
