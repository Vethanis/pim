#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include <stdint.h>

typedef struct fd_s { int32_t handle; } fd_t;
static int32_t fd_isopen(fd_t fd) { return fd.handle >= 0; }

#define fd_stdin  0
#define fd_stdout 1
#define fd_stderr 2

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
    uint32_t    st_dev;
    uint16_t    st_ino;
    uint16_t    st_mode;
    int16_t     st_nlink;
    int16_t     st_uid;
    int16_t     st_guid;
    uint32_t    st_rdev;
    int64_t     st_size;
    int64_t     st_atime;
    int64_t     st_mtime;
    int64_t     st_ctime;
} fd_status_t;

int32_t fd_errno(void);

fd_t fd_create(const char* filename);
fd_t fd_open(const char* filename, int32_t writable);
void fd_close(fd_t* hdl);

int32_t fd_read(fd_t hdl, void* dst, int32_t size);
int32_t fd_write(fd_t hdl, const void* src, int32_t size);

int32_t fd_puts(const char* str, fd_t hdl);
int32_t fd_printf(fd_t hdl, const char* fmt, ...);

int32_t fd_seek(fd_t hdl, int32_t offset);
int32_t fd_tell(fd_t hdl);

void fd_pipe(fd_t* p0, fd_t* p1, int32_t bufferSize);
void fd_stat(fd_t hdl, fd_status_t* status);

static int64_t fd_size(fd_t fd)
{
    fd_status_t status;
    fd_stat(fd, &status);
    return status.st_size;
}

PIM_C_END
