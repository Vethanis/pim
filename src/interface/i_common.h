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

const char* COM_Parse(const char* str);
// returns argv index of parm, or 0 if missing
i32 COM_CheckParm(const char* parm);
const char* COM_GetParm(const char* parm, i32 parmArg);
void COM_Init(const char *basedir);
void COM_InitArgv(i32 argc, const char** argv);
// returns the leaf string of a path, or the input if no slashes exist
const char* COM_SkipPath(const char* pathname, i32 size);
// copies strIn to strOut until a '.' is encountered
void COM_StripExtension(const char *strIn, i32 inSize, char *strOut, i32 outSize);
const char *COM_FileExtension(const char *strIn, i32 size);
// copies the leaf string of a path, without extension, into strOut
void COM_FileBase(const char *strIn, i32 inSize, char *strOut, i32 outSize);
// append the extension to path if the leaf string of path does not contain '.'
void COM_DefaultExtension(char *path, i32 pathSize, const char *ext);
void COM_WriteFile(const char* filename, const void* data, i32 len);
buffer_t COM_LoadStackFile(const char* path, void* buf, i32 sz);
buffer_t COM_LoadTempFile(const char* path);
buffer_t COM_LoadHunkFile(const char* path);
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
