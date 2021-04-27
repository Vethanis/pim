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

#include "interface/i_common.h"

#if QUAKE_IMPL

#include "interface/i_sys.h"
#include "interface/i_globals.h"
#include "interface/i_zone.h"
#include "interface/i_console.h"
#include "interface/i_cmd.h"

#include "allocator/allocator.h"
#include "common/stringutil.h"
#include "common/cmd.h"

#include <stdarg.h>

// ----------------------------------------------------------------------------

//
// in memory
//
typedef struct packfile_s
{
    char name[MAX_QPATH];
    i32 filepos, filelen;
} packfile_t;

typedef struct pack_s
{
    char filename[MAX_OSPATH];
    filehdl_t handle;
    i32 numfiles;
    packfile_t *files;
} pack_t;

//
// on disk
//
typedef struct dpackfile_s
{
    char name[56];
    i32 filepos, filelen;
} dpackfile_t;

typedef struct dpackheader_s
{
    char id[4];
    i32 dirofs;
    i32 dirlen;
} dpackheader_t;

typedef struct SearchPath_s
{
    char* filename;
    pack_t* pack;
} SearchPath;

// ----------------------------------------------------------------------------

const char **g_com_argv;
i32 g_com_argc;
i32 g_com_filesize;

char g_com_cmdline[PIM_PATH];
char g_com_gamedir[PIM_PATH];
char g_com_token[1024];
static char ms_vabuf[1024];

qboolean g_standard_quake, g_rogue, g_hipnotic;
qboolean g_proghack;

qboolean g_msg_badread;
i32 g_msg_readcount;
char g_msg_string[2048];

static i32 ms_pathCount;
static SearchPath* ms_paths;

// ----------------------------------------------------------------------------

static void COM_AddGameDirectory(const char *dir);
static pack_t* COM_LoadPackFile(const char *packfile);

static void SearchPath_Clear(void);
static void SearchPath_AddPack(pack_t* pack);
static void SearchPath_AddPath(const char* str);

// ----------------------------------------------------------------------------

void SZ_Alloc(sizebuf_t* buf, i32 startsize)
{
    ASSERT(!buf->data);
    ASSERT(startsize >= 0);
    startsize = pim_max(startsize, 256);
    startsize += 4;
    buf->data = Perm_Realloc(buf->data, startsize);
    memset(buf->data, 0, startsize);
    buf->cursize = 0;
}

void SZ_Free(sizebuf_t* buf)
{
    Mem_Free(buf->data);
    memset(buf, 0, sizeof(*buf));
}

void SZ_Clear(sizebuf_t* buf)
{
    i32 prevsize = buf->cursize;
    if (prevsize > 0)
    {
        memset(buf->data, 0, prevsize);
    }
    buf->cursize = 0;
}

void* SZ_GetSpace(sizebuf_t* buf, i32 length)
{
    ASSERT(length >= 0);
    const i32 prevsize = buf->cursize;
    buf->cursize = prevsize + length;
    // hidden null terminator for SZ_Print.
    buf->data = Perm_Realloc(buf->data, buf->cursize + 4);
    void* ptr = buf->data + prevsize;
    memset(ptr, 0, length + 4);
    return ptr;
}

void SZ_Write(sizebuf_t* buf, const void* data, i32 length)
{
    memcpy(SZ_GetSpace(buf, length), data, length);
}

void SZ_Print(sizebuf_t* buf, const char* data)
{
    SZ_Write(buf, data, StrLen(data));
}

// ----------------------------------------------------------------------------

void ClearLink(link_t *l)
{
    l->prev = l->next = l;
}
void RemoveLink(link_t *l)
{
    l->next->prev = l->prev;
    l->prev->next = l->next;
}
void InsertLinkBefore(link_t *l, link_t *before)
{
    l->next = before;
    l->prev = before->prev;
    l->prev->next = l;
    l->next->prev = l;
}
void InsertLinkAfter(link_t *l, link_t *after)
{
    l->next = after->next;
    l->prev = after;
    l->prev->next = l;
    l->next->prev = l;
}

// ----------------------------------------------------------------------------

void MSG_WriteChar(sizebuf_t *sb, i32 x)
{
    ASSERT(x <= 0x7f);
    ASSERT(x >= -0x80);
    x = pim_min(x, 0x7f);
    x = pim_max(x, -0x80);
    u8 y = (u8)(x & 0xff);
    SZ_Write(sb, &y, sizeof(y));
}

void MSG_WriteByte(sizebuf_t *sb, i32 x)
{
    ASSERT(x <= 0xff);
    ASSERT(x >= 0x00);
    x = pim_min(x, 0xff);
    x = pim_max(x, 0x00);
    u8 y = (u8)(x & 0xff);
    SZ_Write(sb, &y, sizeof(y));
}

void MSG_WriteShort(sizebuf_t *sb, i32 x)
{
    ASSERT(x <= 0x7fff);
    ASSERT(x >= -0x8000);
    x = pim_min(x, 0x7fff);
    x = pim_max(x, -0x8000);
    u16 y = (u16)(x & 0xffff);
    SZ_Write(sb, &y, sizeof(y));
}

void MSG_WriteLong(sizebuf_t *sb, i32 x)
{
    SZ_Write(sb, &x, sizeof(x));
}

void MSG_WriteFloat(sizebuf_t *sb, float x)
{
    SZ_Write(sb, &x, sizeof(x));
}

void MSG_WriteString(sizebuf_t *sb, const char *x)
{
    x = x ? x : "";
    SZ_Write(sb, x, StrLen(x) + 1);
}

void MSG_WriteCoord(sizebuf_t *sb, float x)
{
    MSG_WriteShort(sb, (i32)(x * 8.0f));
}

void MSG_WriteAngle(sizebuf_t *sb, float x)
{
    i32 angle = (i32)(x * (256.0f / 360.0f));
    angle &= 0xff;
    MSG_WriteByte(sb, angle);
}

void MSG_BeginReading(void)
{
    g_msg_readcount = 0;
    g_msg_badread = false;
}

i32 MSG_ReadChar(void)
{
    i8 c;
    if ((g_msg_readcount + sizeof(c)) <= g_net_message.cursize)
    {
        const void* src = &g_net_message.data[g_msg_readcount];
        memcpy(&c, src, sizeof(c));
        g_msg_readcount += sizeof(c);
        return (i32)c;
    }
    g_msg_badread = true;
    return -1;
}

i32 MSG_ReadByte(void)
{
    u8 c;
    if ((g_msg_readcount + sizeof(c)) <= g_net_message.cursize)
    {
        const void* src = &g_net_message.data[g_msg_readcount];
        memcpy(&c, src, sizeof(c));
        g_msg_readcount += sizeof(c);
        return (i32)c;
    }
    g_msg_badread = true;
    return -1;
}

i32 MSG_ReadShort(void)
{
    i16 c;
    if ((g_msg_readcount + sizeof(c)) <= g_net_message.cursize)
    {
        const void* src = &g_net_message.data[g_msg_readcount];
        memcpy(&c, src, sizeof(c));
        g_msg_readcount += sizeof(c);
        return (i32)c;
    }
    g_msg_badread = true;
    return -1;
}

i32 MSG_ReadLong(void)
{
    i32 c;
    if ((g_msg_readcount + sizeof(c)) <= g_net_message.cursize)
    {
        const void* src = &g_net_message.data[g_msg_readcount];
        memcpy(&c, src, sizeof(c));
        g_msg_readcount += sizeof(c);
        return c;
    }
    g_msg_badread = true;
    return -1;
}

float MSG_ReadFloat(void)
{
    float c;
    if ((g_msg_readcount + sizeof(c)) <= g_net_message.cursize)
    {
        const void* src = &g_net_message.data[g_msg_readcount];
        memcpy(&c, src, sizeof(c));
        g_msg_readcount += sizeof(c);
        return c;
    }
    g_msg_badread = true;
    return -1.0f;
}

const char* MSG_ReadString(void)
{
    const i32 offset = g_msg_readcount;
    i32 size = g_net_message.cursize - offset;
    size = pim_min(size, sizeof(g_msg_string));
    if (size > 0)
    {
        const char* src = (char*)(g_net_message.data) + offset;
        i32 len = StrCpy(g_msg_string, size, src);
        g_msg_readcount += len + 1;
    }
    else
    {
        g_msg_badread = true;
        g_msg_string[0] = 0;
    }
    return g_msg_string;
}

float MSG_ReadCoord(void)
{
    return MSG_ReadShort() * (1.0f / 8.0f);
}

float MSG_ReadAngle(void)
{
    return MSG_ReadChar() * (360.0f / 256.0f);
}

// ----------------------------------------------------------------------------

const char* COM_Parse(const char* data)
{
    return cmd_parse(data, g_com_token, NELEM(g_com_token));
}

// returns argv index of parm, or 0 if missing
i32 COM_CheckParm(const char* parm)
{
    const i32 len = g_com_argc;
    const char *const *items = g_com_argv;
    for (i32 i = 1; i < len; ++i)
    {
        if (items[i])
        {
            if (StrICmp(items[0], PIM_PATH, parm) == 0)
            {
                return i;
            }
        }
    }
    return 0;
}

const char* COM_GetParm(const char* parm, i32 parmArg)
{
    ASSERT(parmArg >= 0);
    i32 iArg = COM_CheckParm(parm);
    if (iArg > 0)
    {
        iArg += parmArg;
        if (iArg < g_com_argc)
        {
            return g_com_argv[iArg + parmArg];
        }
    }
    return NULL;
}

static void COM_Path_f(void)
{
    const i32 len = ms_pathCount;
    const SearchPath* paths = ms_paths;

    Con_Printf("Current search path:\n");
    for (i32 i = 0; i < len; ++i)
    {
        const pack_t* pack = paths[i].pack;
        if (pack)
        {
            Con_Printf("%s (%i files)\n", pack->filename, pack->numfiles);
        }
        else
        {
            Con_Printf("%s\n", paths[i].filename);
        }
    }
}

static void COM_AddGameDirectory(const char *dir)
{
    StrCpy(ARGS(g_com_gamedir), dir);

    // add the directory to the search path
    SearchPath_AddPath(dir);

    // add any pak files in the format pak0.pak pak1.pak, ...
    for (i32 i = 0; ; i++)
    {
        char pakfile[PIM_PATH];
        SPrintf(ARGS(pakfile), "%s/pak%i.pak", dir, i);
        pack_t* pak = COM_LoadPackFile(pakfile);
        if (!pak)
        {
            break;
        }
        SearchPath_AddPack(pak);
    }

    // add the contents of the parms.txt file to the end of the command line

}

static pack_t* COM_LoadPackFile(const char *packfile)
{
    filehdl_t packhandle = Sys_FileOpenRead(packfile);
    if (!Sys_FileIsOpen(packhandle))
    {
        return NULL;
    }

    dpackheader_t header = { 0 };
    Sys_FileRead(packhandle, &header, sizeof(header));
    if (memcmp(header.id, "PACK", 4) != 0)
    {
        Sys_Error("%s is not a packfile", packfile);
    }

    const i32 numpackfiles = header.dirlen / sizeof(dpackfile_t);
    if (numpackfiles < 1)
    {
        Sys_Error("Invalid number of files in packfile %s", packfile);
    }

    packfile_t* newfiles = Hunk_AllocName(sizeof(newfiles[0]) * numpackfiles, "packfile");

    dpackfile_t* info = Perm_Alloc(sizeof(info[0]) * numpackfiles);
    Sys_FileSeek(packhandle, header.dirofs);
    Sys_FileRead(packhandle, info, header.dirlen);
    for (i32 i = 0; i < numpackfiles; i++)
    {
        StrCpy(ARGS(newfiles[i].name), info[i].name);
        newfiles[i].filepos = info[i].filepos;
        newfiles[i].filelen = info[i].filelen;
    }
    Mem_Free(info); info = NULL;

    pack_t* pack = Hunk_Alloc(sizeof(*pack));
    StrCpy(ARGS(pack->filename), packfile);
    pack->handle = packhandle;
    pack->numfiles = numpackfiles;
    pack->files = newfiles;

    Con_Printf("Added packfile %s (%i files)\n", packfile, numpackfiles);
    return pack;
}

void COM_InitFilesystem(void)
{
    //
    // -basedir <path>
    // Overrides the system supplied base directory (under GAMENAME)
    //
    char basedir[PIM_PATH] = { 0 };
    StrCpy(ARGS(basedir), g_host_parms.basedir);
    const char* pBaseDir = COM_GetParm("-basedir", 1);
    if (pBaseDir)
    {
        StrCpy(ARGS(basedir), pBaseDir);
    }

    i32 j = StrLen(basedir);
    if (j > 0)
    {
        if ((basedir[j - 1] == '\\') || (basedir[j - 1] == '/'))
        {
            basedir[j - 1] = 0;
        }
    }

    //
    // start up with GAMENAME by default (id1)
    //
    COM_AddGameDirectory(va("%s/"GAMENAME, basedir));

    if (COM_CheckParm("-g_rogue"))
    {
        COM_AddGameDirectory(va("%s/g_rogue", basedir));
    }
    if (COM_CheckParm("-g_hipnotic"))
    {
        COM_AddGameDirectory(va("%s/g_hipnotic", basedir));
    }

    //
    // -game <gamedir>
    // Adds basedir/gamedir as an override game
    //
    const char* pGameDir = COM_GetParm("-game", 1);
    if (pGameDir)
    {
        COM_AddGameDirectory(va("%s/%s", basedir, pGameDir));
    }

    //
    // -path <dir or packfile> [<dir or packfile>] ...
    // Fully specifies the exact serach path, overriding the generated one
    //
    i32 iPathArg = COM_CheckParm("-path");
    if (iPathArg)
    {
        for (i32 i = iPathArg + 1; i < g_com_argc; ++i)
        {
            const char* arg = g_com_argv[i];
            if (!arg || (arg[0] == '+') || (arg[0] == '-'))
            {
                break;
            }
            if (!StrICmp(COM_FileExtension(arg), 4, "pak"))
            {
                pack_t* pack = COM_LoadPackFile(arg);
                if (pack)
                {
                    SearchPath_AddPack(pack);
                }
                else
                {
                    Sys_Error("Couldn't load packfile: %s", arg);
                }
            }
            else
            {
                SearchPath_AddPath(arg);
            }
        }
    }

    if (COM_CheckParm("-g_proghack"))
    {
        g_proghack = true;
    }
}

void COM_Init(const char *basedir)
{
    Cmd_AddCommand("path", COM_Path_f);

    COM_InitFilesystem();
}

void COM_InitArgv(i32 argc, const char** argv)
{
    g_com_argc = argc;
    g_com_argv = argv;
    g_com_cmdline[0] = 0;
    for (i32 i = 0; i < argc; ++i)
    {
        StrCat(ARGS(g_com_cmdline), argv[i]);
        StrCat(ARGS(g_com_cmdline), " ");
    }
    g_standard_quake = true;
    if (COM_CheckParm("-g_rogue"))
    {
        g_rogue = true;
        g_standard_quake = false;
    }
    if (COM_CheckParm("-g_hipnotic"))
    {
        g_hipnotic = true;
        g_standard_quake = false;
    }
}

const char* COM_SkipPath(const char* pathname)
{
    ASSERT(pathname);
    const char* p = StrRChr(pathname, PIM_PATH, '/');
    if (p)
    {
        return p + 1;
    }
    p = StrRChr(pathname, PIM_PATH, '\\');
    if (p)
    {
        return p + 1;
    }
    return pathname;
}

void COM_StripExtension(const char *strIn, char *strOut)
{
    ASSERT(strIn);
    ASSERT(strOut);
    strOut[0] = 0;
    const char* p = StrRChr(strIn, PIM_PATH, '.');
    if (p)
    {
        isize len = (isize)(p - strIn);
        memcpy(strOut, strIn, len);
        strOut[len] = 0;
    }
    else
    {
        StrCpy(strOut, PIM_PATH, strIn);
    }
}

const char* COM_FileExtension(const char *str)
{
    ASSERT(str);
    const char* p = StrRChr(str, PIM_PATH, '.');
    if (p)
    {
        return p + 1;
    }
    return "";
}

void COM_FileBase(const char *strIn, char *strOut)
{
    ASSERT(strIn);
    ASSERT(strOut);
    StrCpy(strOut, PIM_PATH, COM_SkipPath(strIn));
    char* p = (char*)StrRChr(strOut, PIM_PATH, '.');
    if (p)
    {
        *p = 0;
    }
}

void COM_DefaultExtension(char *path, char *ext)
{
    ASSERT(path);
    ASSERT(ext);
    const char* dot = StrRChr(path, PIM_PATH, '.');
    if (!dot)
    {
        StrCat(path, PIM_PATH, ext);
    }
}

void COM_WriteFile(const char* filename, const void* data, i32 len)
{

}

i32 COM_OpenFile(const char* filename, filehdl_t* hdlOut)
{

}

void COM_CloseFile(filehdl_t file)
{

}

u8* COM_LoadStackFile(const char* path, void* buf, i32 sz)
{

}

u8* COM_LoadTempFile(const char* path)
{

}

u8* COM_LoadHunkFile(const char* path)
{

}

void COM_LoadCacheFile(const char* path, cache_user_t *cu)
{

}

const char* va(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    VSPrintf(ARGS(ms_vabuf), fmt, ap);
    va_end(ap);
    return ms_vabuf;
}

// ----------------------------------------------------------------------------

static void SearchPath_Clear(void)
{
    i32 len = ms_pathCount;
    SearchPath* paths = ms_paths;
    for (i32 i = 0; i < len; ++i)
    {
        if (paths[i].filename)
        {
            Mem_Free(paths[i].filename);
        }
        if (paths[i].pack)
        {
            filehdl_t file = paths[i].pack->handle;
            Sys_FileClose(file);
        }
    }
    Mem_Free(paths);
    ms_pathCount = 0;
    ms_paths = NULL;
}

static void SearchPath_AddPack(pack_t* pack)
{
    if (pack)
    {
        SearchPath path = { 0 };
        path.pack = pack;
        i32 len = ++ms_pathCount;
        ms_paths = Perm_Realloc(ms_paths, len);
        ms_paths[len - 1] = path;
    }
}

static void SearchPath_AddPath(const char* str)
{
    if (str)
    {
        SearchPath path = { 0 };
        path.filename = StrDup(str, EAlloc_Perm);
        i32 len = ++ms_pathCount;
        ms_paths = Perm_Realloc(ms_paths, len);
        ms_paths[len - 1] = path;
    }
}

#endif // QUAKE_IMPL
