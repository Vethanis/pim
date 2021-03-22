#include "io/fmap.h"

#include <Windows.h>
#include <io.h>
#include <string.h>

FileMap FileMap_New(fd_t fd, bool writable)
{
    FileMap result = { 0 };
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

void FileMap_Del(FileMap* fmap)
{
    if (fmap)
    {
        if (FileMap_IsOpen(*fmap))
        {
            UnmapViewOfFile(fmap->ptr);
        }
        memset(fmap, 0, sizeof(*fmap));
    }
}

bool FileMap_Flush(FileMap fmap)
{
    if (FileMap_IsOpen(fmap))
    {
        return FlushViewOfFile(fmap.ptr, fmap.size);
    }
    else
    {
        return false;
    }
}

FileMap FileMap_Open(const char* path, bool writable)
{
    fd_t fd = fd_open(path, writable);
    if (!fd_isopen(fd))
    {
        return (FileMap) { 0 };
    }
    FileMap map = FileMap_New(fd, writable);
    if (!FileMap_IsOpen(map))
    {
        fd_close(&fd);
        return (FileMap) { 0 };
    }
    return map;
}

void FileMap_Close(FileMap* map)
{
    if (map)
    {
        fd_t fd = map->fd;
        FileMap_Del(map);
        fd_close(&fd);
    }
}
