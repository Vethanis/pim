#include "io/fd.h"

#include "common/stringutil.h"

#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#if PLAT_WINDOWS
#include <io.h>
#else
#include <unistd.h>
#define _S_IREAD        S_IREAD
#define _S_IWRITE       S_IWRITE
#define _O_RDWR         O_RDWR
#define _O_CREAT        O_CREAT
#define _O_TRUNC        O_TRUNC
#define _O_RDONLY       O_RDONLY
#define _O_RDWR         O_RDWR
#define _O_BINARY       0
#define _O_NOINHERIT    0
#define _O_SEQUENTIAL   0

#endif // PLAT_WINDOWS

// ----------------------------------------------------------------------------

static i32 NotNeg(i32 x);
static i32 IsZero(i32 x);
// https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/open-wopen?view=msvc-160
static i32 Open(const char* filename, i32 flag, i32 mode);
static i32 Close(i32 hdl);
static i32 Read(i32 hdl, void* dst, u32 size);
static i32 Write(i32 hdl, const void* src, u32 size);
static i32 Seek(i32 hdl, i32 offset, i32 origin);
static i32 FileStatus64(i32 hdl, fd_status_t* desc);
static i32 OpenPipe(i32 handles[2], u32 size, i32 mode);
static i32 Tell(i32 hdl);

enum SpecialHandles
{
    SH_Closed = -1,
    SH_StdIn = 0,
    SH_StdOut = 1,
    SH_StdErr = 2,
};
const fd_t fd_stdin = { SH_StdIn };
const fd_t fd_stdout = { SH_StdOut };
const fd_t fd_stderr = { SH_StdErr };

// ----------------------------------------------------------------------------

bool fd_isopen(fd_t fd)
{
    return fd.handle >= SH_StdIn;
}

fd_t fd_new(const char* filename)
{
    ASSERT(filename);
    const i32 kCreateMode = _S_IREAD | _S_IWRITE;
    const i32 kCreateFlags = _O_RDWR | _O_CREAT | _O_TRUNC | _O_BINARY | _O_NOINHERIT | _O_SEQUENTIAL;

    i32 handle = Open(
        filename,
        kCreateFlags,
        kCreateMode);
    return (fd_t) { handle };
}

fd_t fd_open(const char* filename, bool writable)
{
    ASSERT(filename);
    const i32 kReadFlags = _O_BINARY | _O_NOINHERIT | _O_SEQUENTIAL | _O_RDONLY;
    const i32 kWriteFlags = _O_BINARY | _O_NOINHERIT | _O_SEQUENTIAL | _O_RDWR;
    const i32 kReadMode = _S_IREAD;
    const i32 kWriteMode = _S_IREAD | _S_IWRITE;
    i32 flags = writable ? kWriteFlags : kReadFlags;
    i32 mode = writable ? kWriteMode : kReadMode;
    return (fd_t) { Open(filename, flags, mode) };
}

bool fd_close(fd_t* fd)
{
    ASSERT(fd);
    i32 handle = fd->handle;
    fd->handle = SH_Closed;
    if (handle > SH_StdErr)
    {
        return IsZero(Close(handle)) == 0;
    }
    return false;
}

i32 fd_read(fd_t fd, void* dst, i32 size)
{
    ASSERT(fd_isopen(fd));
    ASSERT(dst || !size);
    ASSERT(size >= 0);
    return NotNeg(Read(fd.handle, dst, (u32)size));
}

i32 fd_write(fd_t fd, const void* src, i32 size)
{
    ASSERT(fd_isopen(fd));
    ASSERT(src || !size);
    ASSERT(size >= 0);
    return NotNeg(Write(fd.handle, src, (u32)size));
}

i32 fd_puts(fd_t fd, const char* str)
{
    ASSERT(str);
    ASSERT(fd_isopen(fd));
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
    ASSERT(fd_isopen(fd));
    char buffer[1024];
    va_list ap;
    va_start(ap, fmt);
    i32 a = VSPrintf(ARGS(buffer), fmt, ap);
    va_end(ap);
    ASSERT(a >= 0);
    i32 b = fd_write(fd, buffer, a);
    ASSERT(a == b);
    return b;
}

bool fd_seek(fd_t fd, i32 offset)
{
    ASSERT(fd_isopen(fd));
    ASSERT(offset >= 0);
    return NotNeg((i32)Seek(fd.handle, offset, 0)) >= 0;
}

i32 fd_tell(fd_t fd)
{
    ASSERT(fd_isopen(fd));
    return NotNeg((i32)Tell(fd.handle));
}

bool fd_pipe(fd_t* fd0, fd_t* fd1, i32 bufferSize)
{
    ASSERT(fd0);
    ASSERT(fd1);
    ASSERT(bufferSize >= 0);
    i32 handles[2] = { -1, -1 };
    bool success = IsZero(OpenPipe(handles, (u32)bufferSize, _O_BINARY)) == 0;
    fd0->handle = handles[0];
    fd1->handle = handles[1];
    return success;
}

bool fd_stat(fd_t fd, fd_status_t* status)
{
    ASSERT(fd_isopen(fd));
    ASSERT(status);
    memset(status, 0, sizeof(*status));
    return IsZero(FileStatus64(fd.handle, status)) == 0;
}

i64 fd_size(fd_t fd)
{
    fd_status_t status;
    fd_stat(fd, &status);
    return status.size;
}

// ----------------------------------------------------------------------------

static i32 NotNeg(i32 x)
{
    ASSERT(x >= 0);
    return x;
}

static i32 IsZero(i32 x)
{
    ASSERT(x == 0);
    return x;
}

#if PLAT_WINDOWS

static i32 Open(const char* filename, i32 flag, i32 mode)
{
    return _open(filename, flag, mode);
}
static i32 Close(i32 hdl)
{
    return _close(hdl);
}
static i32 Read(i32 hdl, void* dst, u32 size)
{
    return _read(hdl, dst, size);
}
static i32 Write(i32 hdl, const void* src, u32 size)
{
    return _write(hdl, src, size);
}
static i32 Seek(i32 hdl, i32 offset, i32 origin)
{
    return _lseek(hdl, offset, origin);
}
static i32 FileStatus64(i32 hdl, fd_status_t* desc)
{
    struct _stat64 x = { 0 };
    i32 rv = _fstat64(hdl, &x);
    desc->size = x.st_size;
    desc->accessTime = x.st_atime;
    desc->modifyTime = x.st_mtime;
    desc->createTime = x.st_ctime;
    return rv;
}
static i32 OpenPipe(i32 handles[2], u32 size, i32 mode)
{
    return _pipe(handles, size, mode);
}
static i32 Tell(i32 hdl)
{
    return _tell(hdl);
}

#else

static i32 Open(const char* filename, i32 flag, i32 mode)
{
    return open(filename, flag, mode);
}
static i32 Close(i32 hdl)
{
    return close(hdl);
}
static i32 Read(i32 hdl, void* dst, u32 size)
{
    return read(hdl, dst, size);
}
static i32 Write(i32 hdl, const void* src, u32 size)
{
    return write(hdl, src, size);
}
static i32 Seek(i32 hdl, i32 offset, i32 origin)
{
    return lseek(hdl, offset, origin);
}
static i32 FileStatus64(i32 hdl, fd_status_t* desc)
{
    memset(desc, 0, sizeof(*desc));
    struct stat x = { 0 };
    i32 rv = _fstat64(hdl, &x);
    desc->size = x.st_size;
    desc->accessTime = x.st_atime;
    desc->modifyTime = x.st_mtime;
    desc->createTime = x.st_ctime;
    return rv;
}
static i32 OpenPipe(i32 handles[2], u32 size, i32 mode)
{
    return pipe(hdl);
}
static i32 Tell(i32 hdl)
{
    return lseek(hdl, 0, SEEK_CUR)
}

#endif // PLAT_WINDOWS
