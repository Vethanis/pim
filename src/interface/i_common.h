#pragma once

/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "interface/i_types.h"
#include <string.h>

extern qboolean standard_quake, rogue, hipnotic;

extern char com_gamedir[PIM_PATH];
extern i32 com_filesize;
extern i32 com_argc;
extern const char **com_argv;

extern char com_token[1024];

extern i32 msg_readcount;
extern qboolean msg_badread; // set if a read goes beyond end of message

void SZ_Alloc(sizebuf_t* buf, i32 startsize);
void SZ_Free(sizebuf_t* buf);
void SZ_Clear(sizebuf_t* buf);
void* SZ_GetSpace(sizebuf_t* buf, i32 length);
void SZ_Write(sizebuf_t* buf, const void* data, i32 length);
// strcats onto the sizebuf
void SZ_Print(sizebuf_t* buf, const char* data);

void ClearLink(link_t *l);
void RemoveLink(link_t *l);
void InsertLinkBefore(link_t *l, link_t *before);
void InsertLinkAfter(link_t *l, link_t *after);

void MSG_WriteChar(sizebuf_t *sb, i32 x);
void MSG_WriteByte(sizebuf_t *sb, i32 x);
void MSG_WriteShort(sizebuf_t *sb, i32 x);
void MSG_WriteLong(sizebuf_t *sb, i32 x);
void MSG_WriteFloat(sizebuf_t *sb, float x);
void MSG_WriteString(sizebuf_t *sb, const char *x);
void MSG_WriteCoord(sizebuf_t *sb, float x);
void MSG_WriteAngle(sizebuf_t *sb, float x);
void MSG_BeginReading(void);
i32 MSG_ReadChar(void);
i32 MSG_ReadByte(void);
i32 MSG_ReadShort(void);
i32 MSG_ReadLong(void);
float MSG_ReadFloat(void);
const char* MSG_ReadString(void);
float MSG_ReadCoord(void);
float MSG_ReadAngle(void);

const char* COM_Parse(const char* str);
// returns argv index of parm, or 0 if missing
i32 COM_CheckParm(const char* parm);
const char* COM_GetParm(const char* parm, i32 parmArg);
void COM_Init(const char *basedir);
void COM_InitArgv(i32 argc, const char** argv);
// returns the leaf string of a path, or the input if no slashes exist
const char* COM_SkipPath(const char* pathname);
// copies strIn to strOut until a '.' is encountered
void COM_StripExtension(const char *strIn, char *strOut);
const char *COM_FileExtension(const char *strIn);
// copies the leaf string of a path, without extension, into strOut
void COM_FileBase(const char *strIn, char *strOut);
// append the extension to path if the leaf string of path does not contain '.'
void COM_DefaultExtension(const char *path, char *ext);
void COM_WriteFile(const char* filename, const void* data, i32 len);
i32 COM_OpenFile(const char* filename, filehdl_t* hdlOut);
void COM_CloseFile(filehdl_t file);
u8* COM_LoadStackFile(const char* path, void* buf, i32 sz);
u8* COM_LoadTempFile(const char* path);
u8* COM_LoadHunkFile(const char* path);
void COM_LoadCacheFile(const char* path, cache_user_t *cu);

//============================================================================

// x64 is little-endian
#define BigShort(x)      ShortSwap((x))
#define LittleShort(x)   (x)
#define BigLong(x)       LongSwap((x))
#define LittleLong(x)    (x)
#define BigFloat(x)      FloatSwap((x))
#define LittleFloat(x)   (x)

pim_inline u16 ShortSwap(u16 x)
{
    u16 b1 = x & 0xff;
    u16 b2 = (x >> 8) & 0xff;
    u16 y = (b1 << 8) | b2;
    return y;
}

pim_inline u32 LongSwap(u32 x)
{
    u32 b1 = x & 0xff;
    u32 b2 = (x >> 8) & 0xff;
    u32 b3 = (x >> 16) & 0xff;
    u32 b4 = (x >> 24) & 0xff;
    u32 y = (b1 << 24) | (b2 << 16) | (b3 << 8) | b4;
    return y;
}

pim_inline float FloatSwap(float x)
{
    // should be a memcpy to avoid violating strict aliasing rules
    u32 u;
    memcpy(&u, &x, 4);
    u = LongSwap(u);
    memcpy(&x, &u, 4);
    return x;
}

//============================================================================

// These are provided as macros pointing back to stdc

#define Q_memset(dest, fill, count) ( memset((dest), (fill), (count)) )
#define Q_memcpy(dest, src, count)  ( memcpy((dest), (src), (count)) )
#define Q_memcmp(m1, m2, count)     ( memcmp((m1), (m2), (count)) )
#define Q_atoi(str)                 ( (i32)atoi((str)) )
#define Q_atof(str)                 ( (float)atof((str)) )
#define Q_strcmp(s1, s2)            ( strcmp((s1), (s2)) )
#define Q_strncmp(s1, s2, count)    ( strncmp((s1), (s2), (count)) )
#define Q_strcasecmp(s1, s2)        ( _stricmp((s1), (s2)) )
#define Q_strncasecmp(s1, s2, n)    ( _strnicmp((s1), (s2), (n)) )
#define Q_strlen(str)               ( (i32)strlen( (str) ) )
#define Q_strcpy(dest, src)         ( strcpy((dest), (src)) )
#define Q_strncpy(dest, src, count) ( strncpy((dest), (src), (count)) )
#define Q_strcat(dest, src)         ( strcat((dest), (src)) )
#define Q_strrchr(s, c)             ( strrchr((s), (c)) )

// Thread and nesting unsafe, but quake is single threaded
const char* va(const char* fmt, ...);
