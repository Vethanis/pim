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

typedef struct I_Common_s
{
    void(*_SZ_Alloc)(sizebuf_t* buf, i32 startsize);
    void(*_SZ_Free)(sizebuf_t* buf);
    void(*_SZ_Clear)(sizebuf_t* buf);
    void*(*_SZ_GetSpace)(sizebuf_t* buf, i32 length);
    void(*_SZ_Write)(sizebuf_t* buf, const void* data, i32 length);
    // strcats onto the sizebuf
    void(*_SZ_Print)(sizebuf_t* buf, const char* data);

    void(*_ClearLink)(link_t *l);
    void(*_RemoveLink)(link_t *l);
    void(*_InsertLinkBefore)(link_t *l, link_t *before);
    void(*_InsertLinkAfter)(link_t *l, link_t *after);

    void(*_MSG_WriteChar)(sizebuf_t *sb, i32 x);
    void(*_MSG_WriteByte)(sizebuf_t *sb, i32 x);
    void(*_MSG_WriteShort)(sizebuf_t *sb, i32 x);
    void(*_MSG_WriteLong)(sizebuf_t *sb, i32 x);
    void(*_MSG_WriteFloat)(sizebuf_t *sb, float x);
    void(*_MSG_WriteString)(sizebuf_t *sb, const char *x);
    void(*_MSG_WriteCoord)(sizebuf_t *sb, float x);
    void(*_MSG_WriteAngle)(sizebuf_t *sb, float x);
    void(*_MSG_BeginReading)(void);
    i32(*_MSG_ReadChar)(void);
    i32(*_MSG_ReadByte)(void);
    i32(*_MSG_ReadShort)(void);
    i32(*_MSG_ReadLong)(void);
    float(*_MSG_ReadFloat)(void);
    const char*(*_MSG_ReadString)(void);
    float(*_MSG_ReadCoord)(void);
    float(*_MSG_ReadAngle)(void);

    char*(*_COM_Parse)(char* str);
    // returns argv index of parm, or 0 if missing
    i32(*_COM_CheckParm)(const char* parm);
    void(*_COM_Init)(const char *basedir);
    void(*_COM_InitArgv)(i32 argc, const char** argv);
    // returns the leaf string of a path, or the input if no slashes exist
    const char*(*_COM_SkipPath)(const char* pathname);
    // copies strIn to strOut until a '.' is encountered
    void(*_COM_StripExtension)(const char *strIn, char *strOut);
    // copies the leaf string of a path, without extension, into strOut
    void(*_COM_FileBase)(const char *strIn, char *strOut);
    // append the extension to path if the leaf string of path does not contain '.'
    void(*_COM_DefaultExtension)(const char *path, char *ext);
    void(*_COM_WriteFile)(const char* filename, const void* data, i32 len);
    i32(*_COM_OpenFile)(const char* filename, filehdl_t* hdlOut);
    void(*_COM_CloseFile)(filehdl_t file);
    i32(*_COM_FileSize)(void);
    u8*(*_COM_LoadStackFile)(const char* path, void* buf, i32 sz);
    u8*(*_COM_LoadTempFile)(const char* path);
    u8*(*_COM_LoadHunkFile)(const char* path);
    void(*_COM_LoadCacheFile)(const char* path, cache_user_t *cu);

} I_Common_t;
extern I_Common_t I_Common;

#define SZ_Alloc(...) I_Common._SZ_Alloc(__VA_ARGS__)
#define SZ_Free(...) I_Common._SZ_Free(__VA_ARGS__)
#define SZ_Clear(...) I_Common._SZ_Clear(__VA_ARGS__)
#define SZ_GetSpace(...) I_Common._SZ_GetSpace(__VA_ARGS__)
#define SZ_Write(...) I_Common._SZ_Write(__VA_ARGS__)
#define SZ_Print(...) I_Common._SZ_Print(__VA_ARGS__)

#define ClearLink(...) I_Common._ClearLink(__VA_ARGS__)
#define RemoveLink(...) I_Common._RemoveLink(__VA_ARGS__)
#define InsertLinkBefore(...) I_Common._InsertLinkBefore(__VA_ARGS__)
#define InsertLinkAfter(...) I_Common._InsertLinkAfter(__VA_ARGS__)
// STRUCT_FROM_LINK removed: area link moved to first field of edict_t

#define MSG_WriteChar(...) I_Common._MSG_WriteChar(__VA_ARGS__)
#define MSG_WriteByte(...) I_Common._MSG_WriteByte(__VA_ARGS__)
#define MSG_WriteShort(...) I_Common._MSG_WriteShort(__VA_ARGS__)
#define MSG_WriteLong(...) I_Common._MSG_WriteLong(__VA_ARGS__)
#define MSG_WriteFloat(...) I_Common._MSG_WriteFloat(__VA_ARGS__)
#define MSG_WriteString(...) I_Common._MSG_WriteString(__VA_ARGS__)
#define MSG_WriteCoord(...) I_Common._MSG_WriteCoord(__VA_ARGS__)
#define MSG_WriteAngle(...) I_Common._MSG_WriteAngle(__VA_ARGS__)

#define MSG_BeginReading(...) I_Common._MSG_BeginReading(__VA_ARGS__)
#define MSG_ReadChar(...) I_Common._MSG_ReadChar(__VA_ARGS__)
#define MSG_ReadByte(...) I_Common._MSG_ReadByte(__VA_ARGS__)
#define MSG_ReadShort(...) I_Common._MSG_ReadShort(__VA_ARGS__)
#define MSG_ReadLong(...) I_Common._MSG_ReadLong(__VA_ARGS__)
#define MSG_ReadFloat(...) I_Common._MSG_ReadFloat(__VA_ARGS__)
#define MSG_ReadString(...) I_Common._MSG_ReadString(__VA_ARGS__)

#define MSG_ReadCoord(...) I_Common._MSG_ReadCoord(__VA_ARGS__)
#define MSG_ReadAngle(...) I_Common._MSG_ReadAngle(__VA_ARGS__)

#define COM_Parse(...) I_Common._COM_Parse(__VA_ARGS__)
#define COM_CheckParm(...) I_Common._COM_CheckParm(__VA_ARGS__)
#define COM_Init(...) I_Common._COM_Init(__VA_ARGS__)
#define COM_InitArgv(...) I_Common._COM_InitArgv(__VA_ARGS__)
#define COM_SkipPath(...) I_Common._COM_SkipPath(__VA_ARGS__)
#define COM_StripExtension(...) I_Common._COM_StripExtension(__VA_ARGS__)
#define COM_FileBase(...) I_Common._COM_FileBase(__VA_ARGS__)
#define COM_DefaultExtension(...) I_Common._COM_DefaultExtension(__VA_ARGS__)
#define COM_OpenFile(...) I_Common._COM_OpenFile(__VA_ARGS__)
#define COM_CloseFile(...) I_Common._COM_CloseFile(__VA_ARGS__)
#define COM_WriteFile(...) I_Common._COM_WriteFile(__VA_ARGS__)
#define COM_LoadStackFile(...) I_Common._COM_LoadStackFile(__VA_ARGS__)
#define COM_LoadTempFile(...) I_Common._COM_LoadTempFile(__VA_ARGS__)
#define COM_LoadHunkFile(...) I_Common._COM_LoadHunkFile(__VA_ARGS__)
#define COM_LoadCacheFile(...) I_Common._COM_LoadCacheFile(__VA_ARGS__)

//============================================================================

// These are provided as inline functions for performance's sake.
// These functions must be unsigned in order to avoid sign-extension bugs.

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
#define va(fmt, ...)                ( sprintf_s(g_vabuf, (fmt), __VA_ARGS__) )
