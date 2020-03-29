#include "io/fmap.h"

#include <Windows.h>
#include <io.h>

static PIM_TLS int32_t ms_errno;

int32_t fmap_errno(void)
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

void fmap_create(fmap_t* fmap, fd_t fd, int32_t writable)
{
    ASSERT(fmap);
    ASSERT(fd_isopen(fd));

    fmap->ptr = NULL;
    fmap->size = 0;

    HANDLE hdl = (HANDLE)_get_osfhandle(fd.handle);
    ASSERT(hdl != INVALID_HANDLE_VALUE);
    if (hdl == INVALID_HANDLE_VALUE)
    {
        ms_errno = 1;
        return;
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
        return;
    }

    void* map = MapViewOfFile(fileMapping, dwDesiredAccess, 0, 0, size);
    CloseHandle(fileMapping);
    fileMapping = NULL;
    ASSERT(map);
    if (!map)
    {
        ms_errno = 1;
        return;
    }

    fmap->ptr = map;
    fmap->size = size;
    return;
}

void fmap_destroy(fmap_t* fmap)
{
    ASSERT(fmap);
    if (fmap_isopen(*fmap))
    {
        UnmapViewOfFile(fmap->ptr);
    }
    fmap->ptr = NULL;
    fmap->size = 0;
}

void fmap_flush(fmap_t fmap)
{
    if (fmap_isopen(fmap))
    {
        FlushViewOfFile(fmap.ptr, fmap.size);
    }
    else
    {
        ms_errno = 1;
    }
}
