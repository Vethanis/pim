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

bool FileStream_IsOpen(FileStream fstr)
{
    return fstr.handle != NULL;
}

FileStream FileStream_Open(const char* filename, const char* mode)
{
    ASSERT(filename);
    ASSERT(mode);
    void* ptr = fopen(filename, mode);
    return (FileStream) { ptr };
}

bool FileStream_Close(FileStream* stream)
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

bool FileStream_Flush(FileStream stream)
{
    FILE* file = stream.handle;
    ASSERT(file);
    return IsZero(fflush(file)) == 0;
}

i32 FileStream_Read(FileStream stream, void* dst, i32 size)
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

i32 FileStream_Write(FileStream stream, const void* src, i32 size)
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

bool FileStream_Gets(FileStream stream, char* dst, i32 size)
{
    FILE* file = stream.handle;
    ASSERT(file);
    ASSERT(dst);
    ASSERT(size > 0);
    return NotNull(fgets(dst, size - 1, file)) != NULL;
}

i32 FileStream_Puts(FileStream stream, const char* src)
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

fd_t FileStream_ToFd(FileStream stream)
{
    FILE* file = stream.handle;
    ASSERT(file);
    return (fd_t) { NotNeg(_fileno(file)) };
}

FileStream Fd_ToFileStream(fd_t* pFD, const char* mode)
{
    ASSERT(pFD);
    i32 fd = pFD->handle;
    pFD->handle = -1;
    ASSERT(fd >= 0);
    FILE* ptr = _fdopen(fd, mode);
    return (FileStream) { NotNull(ptr) };
}

bool FileStream_Seek(FileStream stream, i32 offset)
{
    FILE* file = stream.handle;
    ASSERT(file);
    ASSERT(offset >= 0);
    return NotNeg(fseek(file, offset, SEEK_SET)) >= 0;
}

i32 FileStream_Tell(FileStream stream)
{
    FILE* file = stream.handle;
    ASSERT(file);
    return NotNeg(ftell(file));
}

FileStream FileStream_ProcOpen(const char* cmd, const char* mode)
{
    ASSERT(cmd);
    ASSERT(mode);
    FILE* file = _popen(cmd, mode);
    return (FileStream) { NotNull(file) };
}

bool FileStream_ProcClose(FileStream* pStream)
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

bool FileStream_Stat(FileStream stream, fd_status_t* status)
{
    return fd_stat(FileStream_ToFd(stream), status);
}

i64 FileStream_Size(FileStream stream)
{
    fd_status_t status;
    FileStream_Stat(stream, &status);
    return status.st_size;
}
