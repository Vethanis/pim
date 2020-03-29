#define _CRT_SECURE_NO_WARNINGS
#include "io/fd.h"

#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include "common/stringutil.h"

static PIM_TLS int32_t ms_errno = 0;

int32_t fd_errno(void)
{
    int32_t rv = ms_errno;
    ms_errno = 0;
    return rv;
}

static int32_t NotNeg(int32_t x)
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

static int32_t IsZero(int32_t x)
{
    if (x)
    {
        ms_errno = 1;
    }
    return x;
}

fd_t fd_create(const char* filename)
{
    ASSERT(filename);
    int32_t mode = _S_IREAD | _S_IWRITE;
    int32_t flags = _O_RDWR | _O_CREAT | _O_TRUNC | _O_BINARY | _O_NOINHERIT | _O_SEQUENTIAL;
    int32_t handle = _open(
        filename,
        flags,
        mode);
    return (fd_t) { NotNeg(handle) };
}

fd_t fd_open(const char* filename, int32_t writable)
{
    ASSERT(filename);
    int32_t mode = _S_IREAD;
    int32_t flags = _O_BINARY | _O_NOINHERIT | _O_SEQUENTIAL;
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
    int32_t handle = fd->handle;
    fd->handle = -1;
    if (handle > 2)
    {
        IsZero(_close(handle));
    }
}

int32_t fd_read(fd_t fd, void* dst, int32_t size)
{
    int32_t handle = fd.handle;
    ASSERT(handle >= 0);
    ASSERT(dst);
    ASSERT(size >= 0);
    return NotNeg(_read(handle, dst, (uint32_t)size));
}

int32_t fd_write(fd_t fd, const void* src, int32_t size)
{
    int32_t handle = fd.handle;
    ASSERT(handle >= 0);
    ASSERT(src);
    ASSERT(size >= 0);
    return NotNeg(_write(handle, src, (uint32_t)size));
}

int32_t fd_puts(const char* str, fd_t fd)
{
    ASSERT(str);
    ASSERT(fd.handle >= 0);
    int32_t len = StrLen(str);
    int32_t a = fd_write(fd, str, len);
    ASSERT(a == len);
    int32_t b = 0;
    if (a == len)
    {
        b = fd_write(fd, "\n", 1);
        ASSERT(b == 1);
    }
    return a + b;
}

int32_t fd_printf(fd_t fd, const char* fmt, ...)
{
    ASSERT(fmt);
    ASSERT(fd.handle >= 0);
    char buffer[1024];
    int32_t a = VSPrintf(ARGS(buffer), fmt, VA_START(fmt));
    ASSERT(a >= 0);
    int32_t b = fd_write(fd, buffer, a);
    ASSERT(a == b);
    return b;
}

int32_t fd_seek(fd_t fd, int32_t offset)
{
    ASSERT(fd.handle >= 0);
    return NotNeg((int32_t)_lseek(fd.handle, (int32_t)offset, 0));
}

int32_t fd_tell(fd_t fd)
{
    ASSERT(fd.handle >= 0);
    return NotNeg((int32_t)_tell(fd.handle));
}

void fd_pipe(fd_t* fd0, fd_t* fd1, int32_t bufferSize)
{
    ASSERT(fd0);
    ASSERT(fd1);
    ASSERT(bufferSize >= 0);
    int32_t handles[2] = { -1, -1 };
    IsZero(_pipe(handles, (uint32_t)bufferSize, _O_BINARY));
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
