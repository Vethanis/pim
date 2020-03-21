#define _CRT_SECURE_NO_WARNINGS

#include "common/io.h"
#include "common/stringutil.h"

#if PLAT_WINDOWS
    #include <windows.h>
#endif // PLAT_WINDOWS

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#if PLAT_WINDOWS
    #include <direct.h>
#endif // PLAT_WINDOWS

static PIM_TLS int32_t ms_errno;

int32_t pim_errno(void)
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
    if (x == 0)
    {
        ms_errno = 1;
    }
    return x;
}

static int32_t IsZero(int32_t x)
{
    if (x != 0)
    {
        ms_errno = 1;
    }
    return x;
}

fd_t pim_create(const char* filename)
{
    ASSERT(filename);

    int32_t mode = _S_IREAD | _S_IWRITE;
    int32_t flags = _O_RDWR | _O_CREAT | _O_TRUNC | _O_BINARY | _O_NOINHERIT | _O_SEQUENTIAL;

    int32_t fd = _open(
        filename,
        flags,
        mode);

    return NotNeg(fd);
}

fd_t pim_open(const char* filename, int32_t writable)
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
    return NotNeg(_open(filename, flags, mode));
}

void pim_close(fd_t* pFD)
{
    ASSERT(pFD);
    int32_t fd = *pFD;
    *pFD = -1;
    if (fd > 2)
    {
        IsZero(_close(fd));
    }
}

int32_t pim_read(fd_t fd, void* dst, int32_t size)
{
    ASSERT(fd >= 0);
    ASSERT(dst);
    ASSERT(size >= 0);
    return NotNeg(_read(fd, dst, (uint32_t)size));
}

int32_t pim_write(fd_t fd, const void* src, int32_t size)
{
    ASSERT(fd >= 0);
    ASSERT(src);
    ASSERT(size >= 0);
    return NotNeg(_write(fd, src, (uint32_t)size));
}

int32_t pim_puts(const char* str, fd_t fd)
{
    ASSERT(str);
    ASSERT(fd >= 0);
    int32_t len = StrLen(str);
    int32_t a = pim_write(fd, str, len);
    ASSERT(a == len);
    int32_t b = 0;
    if (a == len)
    {
        b = pim_write(fd, "\n", 1);
        ASSERT(b == 1);
    }
    return a + b;
}

int32_t pim_printf(fd_t fd, const char* fmt, ...)
{
    ASSERT(fmt);
    ASSERT(fd >= 0);
    char buffer[1024];
    int32_t a = VSPrintf(ARGS(buffer), fmt, VA_START(fmt));
    ASSERT(a >= 0);
    int32_t b = pim_write(fd, buffer, a);
    ASSERT(a == b);
    return b;
}

int32_t pim_seek(fd_t fd, int32_t offset)
{
    ASSERT(fd >= 0);
    return NotNeg((int32_t)_lseek(fd, (int32_t)offset, SEEK_SET));
}

int32_t pim_tell(fd_t fd)
{
    ASSERT(fd >= 0);
    return NotNeg((int32_t)_tell(fd));
}

void pim_pipe(fd_t* p0, fd_t* p1, int32_t bufferSize)
{
    int32_t fds[2] = { -1, -1 };
    IsZero(_pipe(fds, (uint32_t)bufferSize, _O_BINARY));
    *p0 = fds[0];
    *p1 = fds[1];
}

void pim_stat(fd_t fd, fd_status_t* status)
{
    ASSERT(fd >= 0);
    ASSERT(status);
    memset(status, 0, sizeof(fd_status_t));
    IsZero(_fstat64(fd, (struct _stat64*)status));
}

// ------------------------------------------------------------------------

fstr_t pim_fopen(const char* filename, const char* mode)
{
    ASSERT(filename);
    ASSERT(mode);
    return NotNull(fopen(filename, mode));
}

void pim_fclose(fstr_t* stream)
{
    ASSERT(stream);
    FILE* file = (FILE*)(*stream);
    *stream = 0;
    if (file)
    {
        IsZero(fclose(file));
    }
}

void pim_fflush(fstr_t stream)
{
    ASSERT(stream);
    IsZero(fflush((FILE*)stream));
}

int32_t pim_fread(fstr_t stream, void* dst, int32_t size)
{
    ASSERT(stream);
    ASSERT(dst);
    ASSERT(size >= 0);
    int32_t ct = (int32_t)fread(dst, 1, size, (FILE*)stream);
    if (ct != size)
    {
        ms_errno = 1;
    }
    return ct;
}

int32_t pim_fwrite(fstr_t stream, const void* src, int32_t size)
{
    ASSERT(stream);
    ASSERT(src);
    ASSERT(size >= 0);
    int32_t ct = (int32_t)fwrite(src, 1, size, (FILE*)stream);
    if (ct != size)
    {
        ms_errno = 1;
    }
    return ct;
}

void pim_fgets(fstr_t stream, char* dst, int32_t size)
{
    ASSERT(stream);
    ASSERT(dst);
    ASSERT(size > 0);
    NotNull(fgets(dst, size - 1, (FILE*)stream));
}

int32_t pim_fputs(fstr_t stream, const char* src)
{
    ASSERT(stream);
    ASSERT(src);
    return NotNeg(fputs(src, (FILE*)stream));
}

fd_t pim_fileno(fstr_t stream)
{
    ASSERT(stream);
    return NotNeg(_fileno((FILE*)stream));
}

fstr_t pim_fdopen(fd_t* pFD, const char* mode)
{
    ASSERT(pFD);
    int32_t fd = *pFD;
    *pFD = -1;
    ASSERT(fd >= 0);
    FILE* ptr = _fdopen(fd, mode);
    return NotNull(ptr);
}

void pim_fseek(fstr_t stream, int32_t offset)
{
    ASSERT(stream);
    ASSERT(offset >= 0);
    NotNeg(fseek((FILE*)stream, offset, SEEK_SET));
}

int32_t pim_ftell(fstr_t stream)
{
    ASSERT(stream);
    return NotNeg(ftell((FILE*)stream));
}

fstr_t pim_popen(const char* cmd, const char* mode)
{
    ASSERT(cmd);
    ASSERT(mode);
    FILE* file = _popen(cmd, mode);
    return NotNull(file);
}

void pim_pclose(fstr_t* pStream)
{
    ASSERT(pStream);
    fstr_t stream = *pStream;
    *pStream = 0;
    FILE* file = (FILE*)stream;
    if (file)
    {
        IsZero(_pclose(file));
    }
}

// ------------------------------------------------------------------------

void pim_getcwd(char* dst, int32_t size)
{
    ASSERT(dst);
    ASSERT(size > 0);
    NotNull(_getcwd(dst, size - 1));
}

void pim_chdir(const char* path)
{
    ASSERT(path);
    IsZero(_chdir(path));
}

void pim_mkdir(const char* path)
{
    ASSERT(path);
    IsZero(_mkdir(path));
}

void pim_rmdir(const char* path)
{
    ASSERT(path);
    IsZero(_rmdir(path));
}

void pim_chmod(const char* path, int32_t flags)
{
    ASSERT(path);
    IsZero(_chmod(path, flags));
}

// ------------------------------------------------------------------------

void pim_searchenv(const char* filename, const char* varname, char* dst)
{
    ASSERT(filename);
    ASSERT(varname);
    ASSERT(dst);
    dst[0] = 0;
    _searchenv(filename, varname, dst);
    if (dst[0] == 0)
    {
        ms_errno = 1;
    }
}

const char* pim_getenv(const char* varname)
{
    ASSERT(varname);
    return (const char*)NotNull(getenv(varname));
}

void pim_putenv(const char* varname, const char* value)
{
    ASSERT(varname);
    char buf[260];
    SPrintf(ARGS(buf), "%s=%s", varname, value ? value : "");
    IsZero(_putenv(buf));
}

// ------------------------------------------------------------------------

// https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/findfirst-functions
// https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/findnext-functions
// https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/findclose

int32_t pim_fndfirst(fnd_t* fdr, fnd_data_t* data, const char* spec)
{
    ASSERT(fdr);
    ASSERT(data);
    ASSERT(spec);
    *fdr = _findfirst64(spec, (struct __finddata64_t*)data);
    return fnd_isopen(*fdr);
}

int32_t pim_fndnext(fnd_t* fdr, fnd_data_t* data)
{
    ASSERT(fdr);
    ASSERT(data);
    if (fnd_isopen(*fdr))
    {
        if (!_findnext64(*fdr, (struct __finddata64_t*)data))
        {
            return 1;
        }
    }
    return 0;
}

void pim_fndclose(fnd_t* fdr)
{
    ASSERT(fdr);
    if (fnd_isopen(*fdr))
    {
        _findclose(*fdr);
        *fdr = -1;
    }
}

int32_t pim_fnditer(fnd_t* fdr, fnd_data_t* data, const char* spec)
{
    ASSERT(fdr);
    if (fnd_isopen(*fdr))
    {
        if (pim_fndnext(fdr, data))
        {
            return 1;
        }
        pim_fndclose(fdr);
        return 0;
    }
    else
    {
        return pim_fndfirst(fdr, data, spec);
    }
}

// ------------------------------------------------------------------------

fmap_t pim_mmap(fd_t fd, int32_t writable)
{
    ASSERT(fd >= 0);

    fmap_t result = { 0, 0 };

    HANDLE hdl = (HANDLE)_get_osfhandle(fd);
    ASSERT(hdl != INVALID_HANDLE_VALUE);
    if (hdl == INVALID_HANDLE_VALUE)
    {
        ms_errno = 1;
        return result;
    }

    int32_t size = (int32_t)fd_size(fd);
    ASSERT(size >= 0);

    int32_t flProtect = writable ? PAGE_READWRITE : PAGE_READONLY;
    int32_t dwDesiredAccess = writable ? (FILE_MAP_READ | FILE_MAP_WRITE) : FILE_MAP_READ;

    HANDLE fileMapping = CreateFileMappingA(hdl, NULL, flProtect, 0, size, NULL);
    ASSERT(fileMapping);
    if (!fileMapping)
    {
        ms_errno = 1;
        return result;
    }

    void* map = MapViewOfFile(fileMapping, dwDesiredAccess, 0, 0, size);
    CloseHandle(fileMapping);
    fileMapping = NULL;
    ASSERT(map);
    if (!map)
    {
        ms_errno = 1;
        return result;
    }

    result.ptr = map;
    result.size = size;

    return result;
}

void pim_munmap(fmap_t* map)
{
    ASSERT(map);
    if (map->ptr)
    {
        UnmapViewOfFile(map->ptr);
    }
    map->ptr = 0;
    map->size = 0;
}

void pim_msync(fmap_t map)
{
    ASSERT(map.ptr);
    if (map.ptr)
    {
        FlushViewOfFile(map.ptr, map.size);
    }
    else
    {
        ms_errno = 1;
    }
}
