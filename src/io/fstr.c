#include "io/fstr.h"
#include "io/fd.h"

#include <stdio.h>
#include <stdarg.h>

#if PLAT_WINDOWS

#ifndef fileno
#   define fileno(f)        _fileno(f)
#endif // fileno
#ifdef fdopen
#   define fdopen(f, m)     _fdopen(f, m)
#endif // fdopen
#ifdef popen
#   define popen(c, t)      _popen(c, t)
#endif // popen
#ifdef pclose
#   define pclose(f)        _pclose(f)
#endif

#endif // PLAT_WINDOWS

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

bool FStream_IsOpen(FStream fstr)
{
    return fstr.handle != NULL;
}

FStream FStream_Open(const char* filename, const char* mode)
{
    ASSERT(filename);
    ASSERT(mode);
    void* ptr = fopen(filename, mode);
    return (FStream) { ptr };
}

bool FStream_Close(FStream* stream)
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

bool FStream_Flush(FStream stream)
{
    FILE* file = stream.handle;
    ASSERT(file);
    return IsZero(fflush(file)) == 0;
}

bool FStream_AtEnd(FStream stream)
{
    FILE* file = stream.handle;
    ASSERT(file);
    return feof(file) != 0;
}

i32 FStream_Read(FStream stream, void* dst, i32 size)
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

i32 FStream_Write(FStream stream, const void* src, i32 size)
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

bool FStream_Gets(FStream stream, char* dst, i32 size)
{
    FILE* file = stream.handle;
    ASSERT(file);
    ASSERT(dst);
    ASSERT(size > 0);
    return NotNull(fgets(dst, size - 1, file)) != NULL;
}

i32 FStream_Getc(FStream stream)
{
    FILE* file = stream.handle;
    ASSERT(file);
    return fgetc(file);
}

i32 FStream_Puts(FStream stream, const char* src)
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

i32 FStream_VPrintf(FStream stream, const char* fmt, va_list ap)
{
    FILE* file = stream.handle;
    ASSERT(file);
    ASSERT(fmt);
    ASSERT(ap);
    i32 rval = NotNeg(vfprintf(file, fmt, ap));
    return rval;
}

i32 FStream_Printf(FStream stream, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    i32 rval = FStream_VPrintf(stream, fmt, ap);
    va_end(ap);
    return rval;
}

i32 FStream_VScanf(FStream stream, const char* fmt, va_list ap)
{
    FILE* file = stream.handle;
    ASSERT(file);
    ASSERT(fmt);
    i32 rval = NotNeg(vfscanf(file, fmt, ap));
    return rval;
}

i32 FStream_Scanf(FStream stream, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    i32 rval = FStream_VScanf(stream, fmt, ap);
    va_end(ap);
    return rval;
}

fd_t FStream_ToFd(FStream stream)
{
    FILE* file = stream.handle;
    ASSERT(file);
    return (fd_t) { NotNeg(fileno(file)) };
}

FStream Fd_ToFStream(fd_t* pFD, const char* mode)
{
    ASSERT(pFD);
    i32 fd = pFD->handle;
    pFD->handle = -1;
    ASSERT(fd >= 0);
    FILE* ptr = fdopen(fd, mode);
    return (FStream) { NotNull(ptr) };
}

bool FStream_Seek(FStream stream, i32 offset)
{
    FILE* file = stream.handle;
    ASSERT(file);
    ASSERT(offset >= 0);
    return NotNeg(fseek(file, offset, SEEK_SET)) >= 0;
}

i32 FStream_Tell(FStream stream)
{
    FILE* file = stream.handle;
    ASSERT(file);
    return NotNeg(ftell(file));
}

FStream FStream_POpen(const char* cmd, const char* mode)
{
    ASSERT(cmd);
    ASSERT(mode);
    FILE* file = popen(cmd, mode);
    return (FStream) { NotNull(file) };
}

bool FStream_PClose(FStream* pStream)
{
    ASSERT(pStream);
    FILE* file = pStream->handle;
    pStream->handle = NULL;
    if (file)
    {
        return IsZero(pclose(file)) == 0;
    }
    return false;
}

bool FStream_Stat(FStream stream, fd_status_t* status)
{
    return fd_stat(FStream_ToFd(stream), status);
}

i64 FStream_Size(FStream stream)
{
    fd_status_t status;
    FStream_Stat(stream, &status);
    return status.st_size;
}
