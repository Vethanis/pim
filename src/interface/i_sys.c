#include "interface/i_sys.h"

#if QUAKE_IMPL

#include "interface/i_host.h"
#include "interface/i_common.h"

#include "io/dir.h"
#include "io/fstr.h"
#include "io/fd.h"
#include "allocator/allocator.h"
#include "containers/idalloc.h"
#include "containers/text.h"
#include "containers/table.h"
#include "common/time.h"
#include "common/stringutil.h"
#include "common/console.h"
#include "threading/sleep.h"

#include <string.h>
#include <stdlib.h>

typedef struct FileDesc_s
{
    FStream stream;
    u64 tick;
    bool writable;
    char path[PIM_PATH];
} FileDesc;

static Table ms_table =
{
    .valueSize = sizeof(FileDesc),
};
static quakeparms_t ms_parms;
static char ms_cwd[PIM_PATH];

static void Sys_NormalizePath(char* dst, i32 size, const char* src)
{
    StrCpy(dst, size, src);
    StrPath(dst, size);
}

bool Sys_FileIsOpen(filehdl_t handle)
{
    return Table_Exists(&ms_table, handle.h);
}

i64 Sys_FileSize(filehdl_t handle)
{
    FileDesc* desc = Table_Get(&ms_table, handle.h);
    if (desc)
    {
        desc->tick = Time_Now();
        return FStream_Size(desc->stream);
    }
    return 0;
}

filehdl_t Sys_FileOpenRead(const char *path)
{
    ASSERT(path);
    filehdl_t hdl = { 0 };
    if (path)
    {
        char normalized[PIM_PATH];
        Sys_NormalizePath(ARGS(normalized), path);

        Guid guid = Guid_HashStr(normalized);
        if (Table_Find(&ms_table, guid, &hdl.h))
        {
            Table_Retain(&ms_table, hdl.h);
            FileDesc* desc = Table_Get(&ms_table, hdl.h);
            ASSERT(FStream_IsOpen(desc->stream));
            ASSERT(StrICmp(ARGS(desc->path), normalized) == 0);
            desc->tick = Time_Now();
            if (desc->writable)
            {
                desc->writable = false;
                FStream_Close(&desc->stream);
                desc->stream = FStream_Open(desc->path, "rb");
                ASSERT(FStream_IsOpen(desc->stream));
            }
        }
        else
        {
            FStream stream = FStream_Open(path, "rb");
            if (FStream_IsOpen(stream))
            {
                FileDesc desc = { 0 };
                desc.stream = stream;
                desc.tick = Time_Now();
                StrCpy(ARGS(desc.path), normalized);
                desc.writable = false;
                if (!Table_Add(&ms_table, guid, &desc, &hdl.h))
                {
                    ASSERT(false);
                }
            }
        }
    }
    return hdl;
}

filehdl_t Sys_FileOpenWrite(const char *path)
{
    ASSERT(path);
    filehdl_t hdl = { 0 };
    if (path)
    {
        char normalized[PIM_PATH];
        Sys_NormalizePath(ARGS(normalized), path);

        Guid guid = Guid_HashStr(normalized);
        if (Table_Find(&ms_table, guid, &hdl.h))
        {
            Table_Retain(&ms_table, hdl.h);
            FileDesc* desc = Table_Get(&ms_table, hdl.h);
            ASSERT(FStream_IsOpen(desc->stream));
            ASSERT(StrICmp(ARGS(desc->path), normalized) == 0);
            desc->tick = Time_Now();
            if (!desc->writable)
            {
                desc->writable = true;
                FStream_Close(&desc->stream);
                desc->stream = FStream_Open(desc->path, "wb");
                ASSERT(FStream_IsOpen(desc->stream));
            }
        }
        else
        {
            FStream stream = FStream_Open(path, "wb");
            if (FStream_IsOpen(stream))
            {
                FileDesc desc = { 0 };
                desc.stream = stream;
                desc.tick = Time_Now();
                StrCpy(ARGS(desc.path), normalized);
                desc.writable = false;
                if (!Table_Add(&ms_table, guid, &desc, &hdl.h))
                {
                    ASSERT(false);
                }
            }
        }
    }
    return hdl;
}

void Sys_FileClose(filehdl_t handle)
{
    FileDesc desc;
    if (Table_Release(&ms_table, handle.h, &desc))
    {
        ASSERT(FStream_IsOpen(desc.stream));
        FStream_Close(&desc.stream);
    }
}

bool Sys_FileSeek(filehdl_t hdl, i32 position)
{
    FileDesc* desc = Table_Get(&ms_table, hdl.h);
    ASSERT(desc);
    if (desc)
    {
        desc->tick = Time_Now();
        return FStream_Seek(desc->stream, position);
    }
    return false;
}

i32 Sys_FileRead(filehdl_t hdl, void *dst, i32 count)
{
    FileDesc* desc = Table_Get(&ms_table, hdl.h);
    ASSERT(desc && !desc->writable);
    ASSERT(dst || !count);
    ASSERT(count >= 0);
    if ((desc) && (dst) && (count > 0) && (!desc->writable))
    {
        desc->tick = Time_Now();
        return FStream_Read(desc->stream, dst, count);
    }
    return 0;
}

i32 Sys_FileWrite(filehdl_t hdl, const void *src, i32 count)
{
    FileDesc* desc = Table_Get(&ms_table, hdl.h);
    ASSERT(desc && desc->writable);
    ASSERT(src || !count);
    ASSERT(count >= 0);
    if ((desc) && (src) && (count > 0) && (desc->writable))
    {
        desc->tick = Time_Now();
        return FStream_Write(desc->stream, src, count);
    }
    return 0;
}

i32 Sys_FileVPrintf(filehdl_t hdl, const char* fmt, va_list ap)
{
    FileDesc* desc = Table_Get(&ms_table, hdl.h);
    ASSERT(desc && desc->writable);
    ASSERT(fmt);
    if ((desc) && (fmt) && (desc->writable))
    {
        desc->tick = Time_Now();
        return FStream_VPrintf(desc->stream, fmt, ap);
    }
    return 0;
}

i32 Sys_FilePrintf(filehdl_t hdl, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    i32 rv = Sys_FileVPrintf(hdl, fmt, ap);
    va_end(ap);
    return rv;
}

i64 Sys_FileTime(const char *path)
{
    fd_status_t status = { 0 };
    status.st_mtime = -1;
    if (path)
    {
        char normalized[PIM_PATH];
        Sys_NormalizePath(ARGS(normalized), path);
        fd_t fd = fd_open(normalized, 0);
        if (fd_isopen(fd))
        {
            fd_stat(fd, &status);
            fd_close(&fd);
        }
    }
    return status.st_mtime;
}

bool Sys_MkDir(const char *path)
{
    ASSERT(path);
    if (path)
    {
        char normalized[PIM_PATH];
        Sys_NormalizePath(ARGS(normalized), path);
        return IO_MkDir(path);
    }
    return false;
}

void Sys_Error(const char *error, ...)
{
    ASSERT(error && error[0]);
    va_list ap;
    va_start(ap, error);
    Con_Logv(LogSev_Error, "qk", error, ap);
    va_end(ap);

    // longjmp back to pim
    Host_Shutdown();
}

void Sys_Printf(const char *fmt, ...)
{
    ASSERT(fmt && fmt[0]);
    va_list ap;
    va_start(ap, fmt);
    Con_Logv(LogSev_Info, "qk", fmt, ap);
    va_end(ap);
}

void Sys_Quit(void)
{
    // longjmp back to pim
    Host_Shutdown();
}

double Sys_FloatTime(void)
{
    return Time_Sec(Time_Now() - Time_AppStart());
}

void Sys_Sleep(void)
{
    Intrin_Sleep(1);
}

void Sys_Init(i32 argc, const char** argv)
{
    IO_GetCwd(ARGS(ms_cwd));
    i32 cwdLen = StrLen(ms_cwd);
    if (ms_cwd[cwdLen] == '/')
    {
        ms_cwd[cwdLen] = 0;
    }
    StrPath(ARGS(ms_cwd));
    ms_parms.basedir = ms_cwd;

    ms_parms.argc = argc;
    ms_parms.argv = argv;
    COM_InitArgv(argc, argv);

    ms_parms.memsize = 16 << 20;
    const char* heapsize = COM_GetParm("-heapsize", 1);
    if (heapsize)
    {
        ms_parms.memsize = Q_atoi(heapsize) * 1024;
        ASSERT(ms_parms.memsize > 0);
    }

    Sys_Printf("Host_Init\n");
    Host_Init(&ms_parms);

    // the rest is Host_Frame(Time_Deltaf())
}

#endif // QUAKE_IMPL
