#include "io/fmap.h"

#include <Windows.h>
#include <io.h>
#include <string.h>

fmap_t fmap_create(fd_t fd, bool writable)
{
    fmap_t result = { 0 };
    if (!fd_isopen(fd))
    {
        ASSERT(false);
        return result;
    }

    HANDLE hdl = (HANDLE)_get_osfhandle(fd.handle);
    if (hdl == INVALID_HANDLE_VALUE)
    {
        // stale or closed file descriptor?
        ASSERT(false);
        return result;
    }

    i32 size = (i32)fd_size(fd);
    if (size < 0)
    {
        // files bigger than 2GB are not supported
        ASSERT(false);
        return result;
    }

    i32 flProtect = writable ? PAGE_READWRITE : PAGE_READONLY;
    i32 dwDesiredAccess = writable ? (FILE_MAP_READ | FILE_MAP_WRITE) : FILE_MAP_READ;

    HANDLE fileMapping = CreateFileMappingA(hdl, NULL, flProtect, 0, size, NULL);
    if (!fileMapping)
    {
        // file descriptor is probably not writable
        ASSERT(false);
        return result;
    }

    void* map = MapViewOfFile(fileMapping, dwDesiredAccess, 0, 0, size);
    CloseHandle(fileMapping);
    fileMapping = NULL;
    if (!map)
    {
        ASSERT(false);
        return result;
    }

    result.ptr = map;
    result.size = size;
    result.fd = fd;

    return result;
}

void fmap_destroy(fmap_t* fmap)
{
    if (fmap)
    {
        if (fmap_isopen(*fmap))
        {
            UnmapViewOfFile(fmap->ptr);
        }
        memset(fmap, 0, sizeof(*fmap));
    }
}

bool fmap_flush(fmap_t fmap)
{
    if (fmap_isopen(fmap))
    {
        return FlushViewOfFile(fmap.ptr, fmap.size);
    }
    else
    {
        return false;
    }
}

fmap_t fmap_open(const char* path, bool writable)
{
    fd_t fd = fd_open(path, writable);
    if (!fd_isopen(fd))
    {
        return (fmap_t) { 0 };
    }
    fmap_t map = fmap_create(fd, writable);
    if (!fmap_isopen(map))
    {
        fd_close(&fd);
        return (fmap_t) { 0 };
    }
    return map;
}

void fmap_close(fmap_t* map)
{
    if (map)
    {
        fd_t fd = map->fd;
        fmap_destroy(map);
        fd_close(&fd);
    }
}
