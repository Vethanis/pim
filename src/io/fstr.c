#include "io/fstr.h"

#include <stdio.h>

static i32 NotNeg(i32 x)
{
    if (x < 0)
    {
        ASSERT(false);
    }
    return x;
}

static void* NotNull(void* x)
{
    if (!x)
    {
        ASSERT(false);
    }
    return x;
}

static i32 IsZero(i32 x)
{
    if (x)
    {
        ASSERT(false);
    }
    return x;
}

fstr_t fstr_open(const char* filename, const char* mode)
{
    ASSERT(filename);
    ASSERT(mode);
    void* ptr = fopen(filename, mode);
    return (fstr_t) { ptr };
}

bool fstr_close(fstr_t* stream)
{
    ASSERT(stream);
    FILE* file = stream->handle;
    stream->handle = NULL;
    if (file)
    {
        return IsZero(fclose(file)) == 0;
    }
    return false;
}

bool fstr_flush(fstr_t stream)
{
    FILE* file = stream.handle;
    ASSERT(file);
    return IsZero(fflush(file)) == 0;
}

i32 fstr_read(fstr_t stream, void* dst, i32 size)
{
    FILE* file = stream.handle;
    ASSERT(file);
    ASSERT(dst || !size);
    ASSERT(size >= 0);
    i32 ct = (i32)fread(dst, size, 1, file);
    if (ct != 1)
    {
        ASSERT(false);
    }
    return (ct == 1) ? size : 0;
}

i32 fstr_write(fstr_t stream, const void* src, i32 size)
{
    FILE* file = stream.handle;
    ASSERT(file);
    ASSERT(src || !size);
    ASSERT(size >= 0);
    i32 ct = (i32)fwrite(src, size, 1, file);
    if (ct != 1)
    {
        ASSERT(false);
    }
    return (ct == 1) ? size : 0;
}

bool fstr_gets(fstr_t stream, char* dst, i32 size)
{
    FILE* file = stream.handle;
    ASSERT(file);
    ASSERT(dst);
    ASSERT(size > 0);
    return NotNull(fgets(dst, size - 1, file)) != NULL;
}

i32 fstr_puts(fstr_t stream, const char* src)
{
    FILE* file = stream.handle;
    ASSERT(file);
    ASSERT(src);
    i32 rval = NotNeg(fputs(src, file));
    if (rval >= 0)
    {
        fputc('\n', file);
    }
    return rval;
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

bool fstr_seek(fstr_t stream, i32 offset)
{
    FILE* file = stream.handle;
    ASSERT(file);
    ASSERT(offset >= 0);
    return NotNeg(fseek(file, offset, SEEK_SET)) >= 0;
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

bool fstr_pclose(fstr_t* pStream)
{
    ASSERT(pStream);
    FILE* file = pStream->handle;
    pStream->handle = NULL;
    if (file)
    {
        return IsZero(_pclose(file)) == 0;
    }
    return false;
}
