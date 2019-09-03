
#include "common/platform.h"

#if PLAT_WINDOWS

#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#include "os/fs.h"
#include "common/macro.h"
#include "common/stringutil.h"
#include "common/random.h"

static u64 ToQWord(u32 lo, u32 hi)
{
    u64 x = hi;
    x <<= 32;
    x |= lo;
    return x;
}

namespace Fs
{
    bool CreateFile(cstr path)
    {
        DebugAssert(path);
        HANDLE h = ::CreateFileA(
            path,
            (GENERIC_READ | GENERIC_WRITE),
            (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE),
            0,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            0);
        if (h == INVALID_HANDLE_VALUE)
        {
            return false;
        }
        ::CloseHandle(h);
        return true;
    }

    bool CreateDir(cstr path)
    {
        DebugAssert(path);
        return ::CreateDirectoryA(path, 0);
    }

    bool RemoveFile(cstr path)
    {
        DebugAssert(path);
        return ::DeleteFileA(path);
    }

    bool RemoveDir(cstr path)
    {
        DebugAssert(path);
        return ::RemoveDirectoryA(path);
    }

    bool Find(cstr path, Results& results)
    {
        DebugAssert(path);

        char catPath[FS_MAX_PATH] = {};
        StrCpy(catPath, path);
        ReplaceChars(catPath, '\\', '/');
        RemoveChar(catPath, EndsWithChar(catPath, '/'));

        WIN32_FIND_DATAA data = {};
        HANDLE h = ::FindFirstFileA(catPath, &data);
        if (h == INVALID_HANDLE_VALUE)
        {
            return false;
        }

        RemoveAfter(catPath, StrRChr(catPath, '/'));
        ToLower(catPath);

        do
        {
            if (EndsWithChar(data.cFileName, '.'))
            {
                continue;
            }

            Info info = {};
            info.attributes.archive = (data.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE) ? 1 : 0;
            info.attributes.compressed = (data.dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED) ? 1 : 0;
            info.attributes.directory = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? 1 : 0;
            info.attributes.encrypted = (data.dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED) ? 1 : 0;
            info.attributes.hidden = (data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) ? 1 : 0;
            info.attributes.normal = (data.dwFileAttributes & FILE_ATTRIBUTE_NORMAL) ? 1 : 0;
            info.attributes.readonly = (data.dwFileAttributes & FILE_ATTRIBUTE_READONLY) ? 1 : 0;
            info.attributes.sparse = (data.dwFileAttributes & FILE_ATTRIBUTE_SPARSE_FILE) ? 1 : 0;
            info.attributes.system = (data.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) ? 1 : 0;
            info.attributes.temporary = (data.dwFileAttributes & FILE_ATTRIBUTE_TEMPORARY) ? 1 : 0;
            info.createTime = ToQWord(data.ftCreationTime.dwLowDateTime, data.ftCreationTime.dwHighDateTime);
            info.lastReadTime = ToQWord(data.ftLastAccessTime.dwLowDateTime, data.ftLastAccessTime.dwHighDateTime);
            info.lastWriteTime = ToQWord(data.ftLastWriteTime.dwLowDateTime, data.ftLastWriteTime.dwHighDateTime);
            info.size = ToQWord(data.nFileSizeLow, data.nFileSizeHigh);

            StrCat(info.path, catPath);
            ToLower(data.cFileName);
            StrCat(info.path, data.cFileName);

            info.extension = 0;

            if (info.attributes.directory)
            {
                results.directories.grow() = info;
            }
            else
            {
                info.extension = StrRChr(info.path, '.');
                results.files.grow() = info;
            }

            data = {};
        } while (::FindNextFileA(h, &data));

        ::FindClose(h);

        return true;
    }
};

#endif // PLAT_WINDOWS
