#define _CRT_SECURE_NO_WARNINGS
#include "io/fstr.h"

#include <stdio.h>

static pim_thread_local i32 ms_errno;

i32 fstr_errno(void)
{
    i32 rv = ms_errno;
    ms_errno = 0;
    return rv;
}

static i32 NotNeg(i32 x)
{
    if (x < 0)
    {
        ms_errno = 1;
    }
    return x;
}

static void* NotNull(void* x)
{
    if (!x)
    {
        ms_errno = 1;
    }
    return x;
}

static i32 IsZero(i32 x)
{
    if (x)
    {
        ms_errno = 1;
    }
    return x;
}

fstr_t fstr_open(const char* filename, const char* mode)
{
    ASSERT(filename);
    ASSERT(mode);
    void* ptr = NotNull(fopen(filename, mode));
    return (fstr_t) { ptr };
}

void fstr_close(fstr_t* stream)
{
    ASSERT(stream);
    FILE* file = stream->handle;
    stream->handle = NULL;
    ASSERT(file);
    if (file)
    {
        IsZero(fclose(file));
    }
}

void fstr_flush(fstr_t stream)
{
    FILE* file = stream.handle;
    ASSERT(file);
    IsZero(fflush(file));
}

i32 fstr_read(fstr_t stream, void* dst, i32 size)
{
    FILE* file = stream.handle;
    ASSERT(file);
    ASSERT(dst);
    ASSERT(size >= 0);
    i32 ct = (i32)fread(dst, 1, size, file);
    if (ct != size)
    {
        ms_errno = 1;
    }
    return ct;
}

i32 fstr_write(fstr_t stream, const void* src, i32 size)
{
    FILE* file = stream.handle;
    ASSERT(file);
    ASSERT(src);
    ASSERT(size >= 0);
    i32 ct = (i32)fwrite(src, 1, size, file);
    if (ct != size)
    {
        ms_errno = 1;
    }
    return ct;
}

void fstr_gets(fstr_t stream, char* dst, i32 size)
{
    FILE* file = stream.handle;
    ASSERT(file);
    ASSERT(dst);
    ASSERT(size > 0);
    NotNull(fgets(dst, size - 1, file));
}

i32 fstr_puts(fstr_t stream, const char* src)
{
    FILE* file = stream.handle;
    ASSERT(file);
    ASSERT(src);
    return NotNeg(fputs(src, file));
}

fd_t fstr_to_fd(fstr_t stream)
{
    FILE* file = stream.handle;
    ASSERT(file);
    return (fd_t) { NotNeg(_fileno(file)) };
}

fstr_t fd_to_fstr(fd_t* pFD, const char* mode)
{
    ASSERT(pFD);
    i32 fd = pFD->handle;
    pFD->handle = -1;
    ASSERT(fd >= 0);
    FILE* ptr = _fdopen(fd, mode);
    return (fstr_t) { NotNull(ptr) };
}

void fstr_seek(fstr_t stream, i32 offset)
{
    FILE* file = stream.handle;
    ASSERT(file);
    ASSERT(offset >= 0);
    NotNeg(fseek(file, offset, SEEK_SET));
}

i32 fstr_tell(fstr_t stream)
{
    FILE* file = stream.handle;
    ASSERT(file);
    return NotNeg(ftell(file));
}

fstr_t fstr_popen(const char* cmd, const char* mode)
{
    ASSERT(cmd);
    ASSERT(mode);
    FILE* file = _popen(cmd, mode);
    return (fstr_t) { NotNull(file) };
}

void fstr_pclose(fstr_t* pStream)
{
    ASSERT(pStream);
    FILE* file = pStream->handle;
    pStream->handle = NULL;
    if (file)
    {
        IsZero(_pclose(file));
    }
}
