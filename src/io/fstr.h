#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include <stdint.h>
#include "io/fd.h"

typedef struct fstr_s { void* handle; } fstr_t;
static int32_t fstr_isopen(fstr_t fstr) { return fstr.handle != NULL; }

int32_t fstr_errno(void);

fstr_t fstr_open(const char* filename, const char* mode);
void fstr_close(fstr_t* stream);
void fstr_flush(fstr_t stream);
int32_t fstr_read(fstr_t stream, void* dst, int32_t size);
int32_t fstr_write(fstr_t stream, const void* src, int32_t size);
void fstr_gets(fstr_t stream, char* dst, int32_t size);
int32_t fstr_puts(fstr_t stream, const char* src);

fd_t fstr_to_fd(fstr_t stream);
fstr_t fd_to_fstr(fd_t* hdl, const char* mode);

void fstr_seek(fstr_t stream, int32_t offset);
int32_t fstr_tell(fstr_t stream);

fstr_t fstr_popen(const char* cmd, const char* mode);
void fstr_pclose(fstr_t* stream);

static void fstr_stat(fstr_t stream, fd_status_t* status)
{
    fd_stat(fstr_to_fd(stream), status);
}

static int64_t fstr_size(fstr_t stream)
{
    fd_status_t status;
    fstr_stat(stream, &status);
    return status.st_size;
}

PIM_C_END
