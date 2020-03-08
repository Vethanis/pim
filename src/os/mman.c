#include "mman.h"

#ifdef _MSC_VER

#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
    #define NOMINMAX
#endif

#include <Windows.h>
#include <errno.h>
#include <io.h>
#include <assert.h>

#define MMAN_64 (UINTPTR_MAX == UINT64_MAX)
#define MMAN_32 (UINTPTR_MAX == UINT32_MAX)

#ifndef FILE_MAP_EXECUTE
    #define FILE_MAP_EXECUTE 0x0020
#endif /* FILE_MAP_EXECUTE */

static DWORD prot_page(int32_t prot)
{
    DWORD protect = 0;
    if (prot & PROT_EXEC)
    {
        protect = (prot & PROT_WRITE) ? PAGE_EXECUTE_READWRITE : PAGE_EXECUTE_READ;
    }
    else
    {
        protect = (prot & PROT_WRITE) ? PAGE_READWRITE : PAGE_READONLY;
    }
    return protect;
}

static DWORD prot_file(int32_t prot)
{
    DWORD desiredAccess = 0;
    desiredAccess |= (prot & PROT_READ) ? FILE_MAP_READ : 0;
    desiredAccess |= (prot & PROT_WRITE) ? FILE_MAP_WRITE : 0;
    desiredAccess |= (prot & PROT_EXEC) ? FILE_MAP_EXECUTE : 0;
    return desiredAccess;
}

void* mmap(void* addr, size_t len, int32_t prot, int32_t flags, int32_t fd, intptr_t off)
{
    errno = 0;

    HANDLE fm = INVALID_HANDLE_VALUE;
    HANDLE h = INVALID_HANDLE_VALUE;

    if (!len || (prot == PROT_EXEC))
    {
        errno = EINVAL;
        return MAP_FAILED;
    }

    if (!(flags & MAP_ANONYMOUS))
    {
        h = (HANDLE)_get_osfhandle(fd);
        if (h == INVALID_HANDLE_VALUE)
        {
            errno = EBADF;
            return MAP_FAILED;
        }
    }

    const intptr_t maxSize = off + (intptr_t)len;
    const DWORD dwMaxSizeLow = (DWORD)(maxSize & 0xFFFFFFFF);
    const DWORD dwMaxSizeHigh = (DWORD)((maxSize >> 32) & 0xFFFFFFFF);
    const DWORD prpage = prot_page(prot);
    const DWORD prfile = prot_file(prot);

    fm = CreateFileMappingA(h, NULL, prpage, dwMaxSizeHigh, dwMaxSizeLow, NULL);
    if (!fm)
    {
        errno = EINVAL;
        return MAP_FAILED;
    }

    const DWORD dwFileOffsetLow = (DWORD)(off & 0xFFFFFFFF);
    const DWORD dwFileOffsetHigh = (DWORD)((off >> 32) & 0xFFFFFFFF);
    void* map = NULL;
    if (flags & MAP_FIXED)
    {
        map = MapViewOfFileEx(fm, prfile, dwFileOffsetHigh, dwFileOffsetLow, len, addr);
    }
    else
    {
        map = MapViewOfFile(fm, prfile, dwFileOffsetHigh, dwFileOffsetLow, len);
    }

    CloseHandle(fm);
    fm = NULL;

    if (!map)
    {
        errno = EINVAL;
        return MAP_FAILED;
    }

    return map;
}

int32_t munmap(void *addr, size_t len)
{
    if (UnmapViewOfFile(addr))
    {
        return 0;
    }

    errno = EINVAL;

    return -1;
}

int32_t _mprotect(void *addr, size_t len, int32_t prot)
{
    DWORD newProtect = prot_page(prot);
    DWORD oldProtect = 0;
    if (VirtualProtect(addr, len, newProtect, &oldProtect))
    {
        return 0;
    }

    errno = EINVAL;

    return -1;
}

int32_t msync(void *addr, size_t len, int32_t flags)
{
    if (FlushViewOfFile(addr, len))
    {
        return 0;
    }

    errno = EINVAL;

    return -1;
}

int32_t mlock(const void *addr, size_t len)
{
    if (VirtualLock((LPVOID)addr, len))
    {
        return 0;
    }

    errno = EINVAL;

    return -1;
}

int32_t munlock(const void *addr, size_t len)
{
    if (VirtualUnlock((LPVOID)addr, len))
    {
        return 0;
    }

    errno = EINVAL;

    return -1;
}

#endif // _MSC_VER
