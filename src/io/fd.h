#pragma once

#include "io/types.h"

PIM_C_BEGIN

extern const fd_t fd_stdin;
extern const fd_t fd_stdout;
extern const fd_t fd_stderr;

bool fd_isopen(fd_t fd);
fd_t fd_new(const char* filename);
fd_t fd_open(const char* filename, bool writable);
bool fd_close(fd_t* hdl);

i32 fd_read(fd_t hdl, void* dst, i32 size);
i32 fd_write(fd_t hdl, const void* src, i32 size);

i32 fd_puts(fd_t hdl, const char* str);
i32 fd_printf(fd_t hdl, const char* fmt, ...);

bool fd_seek(fd_t hdl, i32 offset);
i32 fd_tell(fd_t hdl);

bool fd_pipe(fd_t* p0, fd_t* p1, i32 bufferSize);
bool fd_stat(fd_t hdl, fd_status_t* status);
i64 fd_size(fd_t hdl);

PIM_C_END
