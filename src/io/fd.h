#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct fd_s { i32 handle; } fd_t;
pim_inline bool fd_isopen(fd_t fd) { return fd.handle >= 0; }

static const fd_t fd_stdin = { 0 };
static const fd_t fd_stdout = { 1 };
static const fd_t fd_stderr = { 2 };

typedef enum
{
    StMode_Regular = 0x8000,
    StMode_Dir = 0x4000,
    StMode_SpecChar = 0x2000,
    StMode_Pipe = 0x1000,
    StMode_Read = 0x0100,
    StMode_Write = 0x0080,
    StMode_Exec = 0x0040,
} StModeFlags;

typedef struct fd_status_s
{
    u32 st_dev;
    u16 st_ino;
    u16 st_mode;
    i16 st_nlink;
    i16 st_uid;
    i16 st_guid;
    u32 st_rdev;
    i64 st_size;
    i64 st_atime;
    i64 st_mtime;
    i64 st_ctime;
} fd_status_t;

fd_t fd_create(const char* filename);
fd_t fd_open(const char* filename, i32 writable);
bool fd_close(fd_t* hdl);

i32 fd_read(fd_t hdl, void* dst, i32 size);
i32 fd_write(fd_t hdl, const void* src, i32 size);

i32 fd_puts(fd_t hdl, const char* str);
i32 fd_printf(fd_t hdl, const char* fmt, ...);

bool fd_seek(fd_t hdl, i32 offset);
i32 fd_tell(fd_t hdl);

bool fd_pipe(fd_t* p0, fd_t* p1, i32 bufferSize);
bool fd_stat(fd_t hdl, fd_status_t* status);

pim_inline i64 fd_size(fd_t fd)
{
    fd_status_t status;
    fd_stat(fd, &status);
    return status.st_size;
}

PIM_C_END
