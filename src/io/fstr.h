#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "io/fd.h"

typedef struct fstr_s { void* handle; } fstr_t;
pim_inline i32 fstr_isopen(fstr_t fstr) { return fstr.handle != NULL; }

fstr_t fstr_open(const char* filename, const char* mode);
void fstr_close(fstr_t* stream);
void fstr_flush(fstr_t stream);
i32 fstr_read(fstr_t stream, void* dst, i32 size);
i32 fstr_write(fstr_t stream, const void* src, i32 size);
void fstr_gets(fstr_t stream, char* dst, i32 size);
i32 fstr_puts(fstr_t stream, const char* src);

fd_t fstr_to_fd(fstr_t stream);
fstr_t fd_to_fstr(fd_t* hdl, const char* mode);

void fstr_seek(fstr_t stream, i32 offset);
i32 fstr_tell(fstr_t stream);

fstr_t fstr_popen(const char* cmd, const char* mode);
void fstr_pclose(fstr_t* stream);

pim_inline void fstr_stat(fstr_t stream, fd_status_t* status)
{
    fd_stat(fstr_to_fd(stream), status);
}

pim_inline i64 fstr_size(fstr_t stream)
{
    fd_status_t status;
    fstr_stat(stream, &status);
    return status.st_size;
}

PIM_C_END
