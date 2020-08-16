#include "io/fd.h"

#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include "common/stringutil.h"

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

fd_t fd_create(const char* filename)
{
    ASSERT(filename);
    i32 mode = _S_IREAD | _S_IWRITE;
    i32 flags = _O_RDWR | _O_CREAT | _O_TRUNC | _O_BINARY | _O_NOINHERIT | _O_SEQUENTIAL;
    i32 handle = _open(
        filename,
        flags,
        mode);
    return (fd_t) { NotNeg(handle) };
}

fd_t fd_open(const char* filename, i32 writable)
{
    ASSERT(filename);
    i32 mode = _S_IREAD;
    i32 flags = _O_BINARY | _O_NOINHERIT | _O_SEQUENTIAL;
    if (writable)
    {
        mode |= _S_IWRITE;
        flags |= _O_RDWR;
    }
    else
    {
        flags |= _O_RDONLY;
    }
    return (fd_t) { NotNeg(_open(filename, flags, mode)) };
}

void fd_close(fd_t* fd)
{
    ASSERT(fd);
    i32 handle = fd->handle;
    fd->handle = -1;
    if (handle > 2)
    {
        IsZero(_close(handle));
    }
}

i32 fd_read(fd_t fd, void* dst, i32 size)
{
    i32 handle = fd.handle;
    ASSERT(handle >= 0);
    ASSERT(dst || !size);
    ASSERT(size >= 0);
    return NotNeg(_read(handle, dst, (u32)size));
}

i32 fd_write(fd_t fd, const void* src, i32 size)
{
    i32 handle = fd.handle;
    ASSERT(handle >= 0);
    ASSERT(src || !size);
    ASSERT(size >= 0);
    return NotNeg(_write(handle, src, (u32)size));
}

i32 fd_puts(fd_t fd, const char* str)
{
    ASSERT(str);
    ASSERT(fd.handle >= 0);
    i32 len = StrLen(str);
    i32 a = fd_write(fd, str, len);
    ASSERT(a == len);
    i32 b = 0;
    if (a == len)
    {
        b = fd_write(fd, "\n", 1);
        ASSERT(b == 1);
    }
    return a + b;
}

i32 fd_printf(fd_t fd, const char* fmt, ...)
{
    ASSERT(fmt);
    ASSERT(fd.handle >= 0);
    char buffer[1024];
    i32 a = VSPrintf(ARGS(buffer), fmt, VA_START(fmt));
    ASSERT(a >= 0);
    i32 b = fd_write(fd, buffer, a);
    ASSERT(a == b);
    return b;
}

i32 fd_seek(fd_t fd, i32 offset)
{
    ASSERT(fd.handle >= 0);
    ASSERT(offset >= 0);
    return NotNeg((i32)_lseek(fd.handle, offset, 0));
}

i32 fd_tell(fd_t fd)
{
    ASSERT(fd.handle >= 0);
    return NotNeg((i32)_tell(fd.handle));
}

void fd_pipe(fd_t* fd0, fd_t* fd1, i32 bufferSize)
{
    ASSERT(fd0);
    ASSERT(fd1);
    ASSERT(bufferSize >= 0);
    i32 handles[2] = { -1, -1 };
    IsZero(_pipe(handles, (u32)bufferSize, _O_BINARY));
    fd0->handle = handles[0];
    fd1->handle = handles[1];
}

void fd_stat(fd_t fd, fd_status_t* status)
{
    ASSERT(fd.handle >= 0);
    ASSERT(status);
    memset(status, 0, sizeof(fd_status_t));
    IsZero(_fstat64(fd.handle, (struct _stat64*)status));
}
