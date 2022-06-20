#include "io/fmap.h"
#include "io/fd.h"
#include <string.h>

// ----------------------------------------------------------------------------

bool FileMap_IsOpen(FileMap* map)
{
    return map->ptr != NULL && map->ptr != (void*)-1;
}

FileMap FileMap_Open(const char* path, bool writable)
{
    fd_t fd = fd_open(path, writable);
    if (!fd_isopen(fd))
    {
        return (FileMap) { 0 };
    }
    FileMap map = FileMap_New(fd, writable);
    if (!FileMap_IsOpen(&map))
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

#if PLAT_WINDOWS
// ----------------------------------------------------------------------------
// Windows

#include <Windows.h>
#include <io.h>

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

    const i32 kRwProt = PAGE_READWRITE;
    const i32 kReadProt = PAGE_READONLY;
    const i32 kReadAcc = FILE_MAP_READ;
    const i32 kWriteAcc = FILE_MAP_READ | FILE_MAP_WRITE;
    i32 flProtect = writable ? kRwProt : kReadProt;
    i32 dwDesiredAccess = writable ? kWriteAcc : kReadAcc;

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
        if (FileMap_IsOpen(fmap))
        {
            UnmapViewOfFile(fmap->ptr);
        }
        fmap->ptr = NULL;
        fmap->size = 0;
    }
}

bool FileMap_Flush(FileMap* fmap)
{
    if (FileMap_IsOpen(fmap))
    {
        return FlushViewOfFile(fmap->ptr, fmap->size);
    }
    else
    {
        return false;
    }
}

#else
// ----------------------------------------------------------------------------
// POSIX

#include <sys/mman.h>
#include "common/console.h"

FileMap FileMap_New(fd_t fd, bool writable)
{
    FileMap result = { 0 };
    if (!fd_isopen(fd))
    {
        ASSERT(false);
        return result;
    }

    const i32 size = (i32)fd_size(fd);
    if (size < 0)
    {
        // files bigger than 2GB are not supported
        ASSERT(false);
        return result;
    }

    // https://man7.org/linux/man-pages/man2/mmap.2.html
    const i32 kWriteProt = PROT_READ | PROT_WRITE;
    const i32 kReadProt = PROT_READ;
    const i32 prot = writable ? kWriteProt : kReadProt;
    const i32 flags = MAP_PRIVATE;
    const i32 offset = 0;
    void* addr = mmap(NULL, size, prot, flags, fd.handle, offset);

    result.ptr = addr;
    result.size = size;
    result.fd = fd;

    return result;
}

void FileMap_Del(FileMap* fmap)
{
    if (fmap)
    {
        if (FileMap_IsOpen(fmap))
        {
            munmap(fmap->ptr, fmap->size);
        }
        fmap->ptr = NULL;
        fmap->size = 0;
    }
}

bool FileMap_Flush(FileMap* fmap)
{
    if (FileMap_IsOpen(fmap))
    {
        // https://man7.org/linux/man-pages/man2/msync.2.html
        // https://stackoverflow.com/a/47915517
        return msync(fmap->ptr, fmap->size, MS_SYNC) == 0;
    }
    else
    {
        return false;
    }
}

#endif // PLAT_X
