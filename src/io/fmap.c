#include "io/fmap.h"

#include <Windows.h>
#include <io.h>

static pim_thread_local i32 ms_errno;

i32 fmap_errno(void)
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

void fmap_create(fmap_t* fmap, fd_t fd, i32 writable)
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

    i32 size = (i32)fd_size(fd);
    ASSERT(size >= 0);

    i32 flProtect = writable ? PAGE_READWRITE : PAGE_READONLY;
    i32 dwDesiredAccess = writable ? (FILE_MAP_READ | FILE_MAP_WRITE) : FILE_MAP_READ;

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
