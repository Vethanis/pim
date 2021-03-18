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

//============================================================================

typedef struct SZ_s
{
    void(*Alloc)(sizebuf_t* buf, i32 startsize);
    void(*Free)(sizebuf_t* buf);
    void(*Clear)(sizebuf_t* buf);
    void*(*GetSpace)(sizebuf_t* buf, i32 length); // reallocates if needed
    void(*Write)(sizebuf_t* buf, const void* data, i32 length);
    void(*Print)(sizebuf_t* buf, const char* data); // strcats onto the sizebuf
} SZ_t;
extern SZ_t SZ;

#define SZ_Alloc(buf, startsize)    SZ.Alloc((buf), (startsize))
#define SZ_Free(buf)                SZ.Free((buf))
#define SZ_Clear(buf)               SZ.Clear((buf))
#define SZ_GetSpace(buf, length)    SZ.GetSpace((buf), (length))
#define SZ_Write(buf, data, length) SZ.Write((buf), (data), (length))
#define SZ_Print(buf, data)         SZ.Print((buf), (data))

//============================================================================

// DEPRECATED (use an array)
// Only used by world.c for edict_t
typedef struct LNK_s
{
    void(*Clear)(link_t *l);
    void(*Remove)(link_t *l);
    void(*InsertBefore)(link_t *l, link_t *before);
    void(*InsertAfter)(link_t *l, link_t *after);
} LNK_t;
extern LNK_t LNK;

#define ClearLink(l)                LNK.Clear((l))
#define RemoveLink(l)               LNK.Remove((l))
#define InsertLinkBefore(l, before) LNK.InsertBefore((l), (before))
#define InsertLinkAfter(l, after)   LNK.InsertAfter((l), (after))

// STRUCT_FROM_LINK removed: area link moved to first field of edict_t

//============================================================================

// x64 is little-endian
#define BigShort(x)      ShortSwap((x))
#define LittleShort(x)   (x)
#define BigLong(x)       LongSwap((x))
#define LittleLong(x)    (x)
#define BigFloat(x)      FloatSwap((x))
#define LittleFloat(x)   (x)

// These functions must be unsigned in order to avoid sign-extension bugs.
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
    u32 u;
    memcpy(&u, &x, 4);
    u = LongSwap(u);
    memcpy(&x, &u, 4);
    return x;
}

//============================================================================

typedef struct MSG_s
{
    void(*WriteChar)(sizebuf_t *sb, i32 x);
    void(*WriteByte)(sizebuf_t *sb, i32 x);
    void(*WriteShort)(sizebuf_t *sb, i32 x);
    void(*WriteLong)(sizebuf_t *sb, i32 x);
    void(*WriteFloat)(sizebuf_t *sb, float x);
    void(*WriteString)(sizebuf_t *sb, const char *x);
    void(*WriteCoord)(sizebuf_t *sb, float x);
    void(*WriteAngle)(sizebuf_t *sb, float x);

    i32(*ReadCount)(void);
    bool(*BadRead)(void);

    void(*BeginReading)(void);
    i32(*ReadChar)(void);
    i32(*ReadByte)(void);
    i32(*ReadShort)(void);
    i32(*ReadLong)(void);
    float(*ReadFloat)(void);
    const char*(*ReadString)(void);

    float(*ReadCoord)(void);
    float(*ReadAngle)(void);
} MSG_t;
extern MSG_t MSG;

#define MSG_WriteChar(sb, x)    MSG.WriteChar((sb), (x))
#define MSG_WriteByte(sb, x)    MSG.WriteByte((sb), (x))
#define MSG_WriteShort(sb, x)   MSG.WriteShort((sb), (x))
#define MSG_WriteLong(sb, x)    MSG.WriteLong((sb), (x))
#define MSG_WriteFloat(sb, x)   MSG.WriteFloat((sb), (x))
#define MSG_WriteString(sb, x)  MSG.WriteString((sb), (x))
#define MSG_WriteCoord(sb, x)   MSG.WriteCoord((sb), (x))
#define MSG_WriteAngle(sb, x)   MSG.WriteAngle((sb), (x))

#define msg_readcount           (MSG.ReadCount())
#define msg_badread             (MSG.BadRead())

#define MSG_BeginReading()      MSG.BeginReading()
#define MSG_ReadChar()          (MSG.ReadChar())
#define MSG_ReadByte()          (MSG.ReadByte())
#define MSG_ReadShort()         (MSG.ReadShort())
#define MSG_ReadLong()          (MSG.ReadLong())
#define MSG_ReadFloat()         (MSG.ReadFloat())
#define MSG_ReadString()        (MSG.ReadString())

#define MSG_ReadCoord()         (MSG.ReadCoord())
#define MSG_ReadAngle()         (MSG.ReadAngle())

//============================================================================

// The compiler knows how to optimize these away, so don't write custom ones.
#define Q_memset(dest, fill, count) memset((dest), (fill), (count))
#define Q_memcpy(dest, src, count)  memcpy((dest), (src), (count))
#define Q_memcmp(m1, m2, count)     memcmp((m1), (m2), (count))

#define Q_atoi(str)                 ( (i32)atoi((str)) )
#define Q_atof(str)                 ( (float)atof((str)) )

// unsafe! deprecated!
#define Q_strcmp(s1, s2)            ( strcmp((s1), (s2)) )
// unsafe! deprecated!
#define Q_strncmp(s1, s2, count)    ( strncmp((s1), (s2), (count)) )
// unsafe! deprecated!
#define Q_strcasecmp(s1, s2)        ( _stricmp((s1), (s2)) )
// unsafe! deprecated!
#define Q_strncasecmp(s1, s2, n)    ( _strnicmp((s1), (s2), (n)) )

// unsafe! deprecated!
#define Q_strlen(str)               ( (i32)strlen( (str) ) )
// unsafe! deprecated!
#define Q_strcpy(dest, src)         ( strcpy((dest), (src)) )
// unsafe! deprecated!
#define Q_strncpy(dest, src, count) ( strncpy((dest), (src), (count)) )
// unsafe! deprecated!
#define Q_strcat(dest, src)         ( strcat((dest), (src)) )

// unsafe! deprecated!
#define Q_strrchr(s, c)             ( strrchr((s), (c)) )

//============================================================================

typedef struct COM_s
{
    const char*(*Token)(void);
    char*(*Parse)(char* str);

    // returns argv index of parm, or 0 if missing
    i32(*CheckParm)(const char* parm);

    void(*Init)(const char *basedir);
    void(*InitArgv)(i32 argc, const char** argv);

    // returns the leaf string of a path, or the input if no slashes exist
    const char*(*SkipPath)(const char* pathname);
    // copies strIn to strOut until a '.' is encountered
    void(*StripExtension)(const char *strIn, char *strOut);
    // copies the leaf string of a path, without extension, into strOut
    void(*FileBase)(const char *strIn, char *strOut);
    // append the extension to path if the leaf string of path does not contain '.'
    void(*DefaultExtension)(const char *path, char *ext);

    void(*WriteFile)(const char* filename, const void* data, i32 len);
    i32(*OpenFile)(const char* filename, filehdl_t* hdlOut);
    void(*CloseFile)(filehdl_t file);

    i32(*FileSize)(void);
    byte*(*LoadStackFile)(const char* path, void* buf, i32 sz);
    byte*(*LoadTempFile)(const char* path);
    byte*(*LoadHunkFile)(const char* path);
    void(*LoadCacheFile)(const char* path, cache_user_t *cu);

    i32(*Argc)(void);
    const char**(*Argv)(void);
    // host and vcr sometimes set this
    void(*SetArgs)(const char** argv, i32 argc);

    const char*(*GameDir)(void);
    bool(*StandardQuake)(void);
    bool(*Rogue)(void);
    bool(*Hipnotic)(void);
} COM_t;
extern COM_t COM;

#define COM_Parse(str)                          COM.Parse((str))
#define COM_CheckParm(parm)                     COM.CheckParm((parm))
#define COM_Init(basedir)                       COM.Init((basedir))
#define COM_InitArgv(argc, argv)                COM.InitArgv((argc), (argv))
#define COM_SkipPath(pathname)                  COM.SkipPath((pathname))
#define COM_StripExtension(strIn, strOut)       COM.StripExtension((strIn), (strOut))
#define COM_FileBase(strIn, strOut)             COM.FileBase((strIn), (strOut))
#define COM_DefaultExtension(path, ext)         COM.DefaultExtension((path), (ext))
#define COM_OpenFile(filename, hdlOut)          COM.OpenFile((filename), (hdlOut))
#define COM_CloseFile(file)                     COM.CloseFile((file))
#define COM_WriteFile(filename, data, len)      COM.WriteFile((filename), (data), (len))
#define COM_LoadStackFile(path, buf, sz)        COM.LoadStackFile((path), (buf), (sz))
#define COM_LoadTempFile(path)                  COM.LoadTempFile((path))
#define COM_LoadHunkFile(path)                  COM.LoadHunkFile((path))
#define COM_LoadCacheFile(path, cu)             COM.LoadCacheFile((path), (cu))

#define com_token       (COM.Token())
#define com_argc        (COM.Argc())
#define com_argv        (COM.Argv())
#define com_filesize    (COM.FileSize())
#define com_gamedir     (COM.GameDir())
#define standard_quake  (COM.StandardQuake())
#define rogue           (COM.Rogue())
#define hipnotic        (COM.Hipnotic())

// not recommended
extern char g_vabuf[1024];
#define va(fmt, ...)    (sprintf_s(g_vabuf, (fmt), __VA_ARGS__))

//============================================================================
