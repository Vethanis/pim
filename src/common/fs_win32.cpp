
#include "common/platform.h"

#if PLAT_WINDOWS

#include "common/fs.h"
#include "common/memory.h"
#include "common/macro.h"
#include "common/stringutil.h"

#include <windows.h>

static void GetErrorString(char* buffer, u32 bufLen)
{
    bufLen = bufLen > 0 ? bufLen - 1 : 0;
    bool worked = FormatMessage(
        FORMAT_MESSAGE_FROM_SYSTEM, 0, GetLastError(),
        1033, buffer, bufLen, 0);
    DebugAssert(worked); (void)worked;
}

static u64 CreateQWord(u32 lo, u32 hi)
{
    u64 x = hi;
    x <<= 32;
    x |= lo;
    return x;
}

namespace Fs
{
    bool CreateFile(const char* path)
    {
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
    bool CreateDir(const char* path)
    {
        return ::CreateDirectoryA(path, 0);
    }
    bool RemoveFile(const char* path)
    {
        return ::DeleteFileA(path);
    }
    bool RemoveDir(const char* path)
    {
        return ::RemoveDirectoryA(path);
    }

    bool Find(const char* path, Results& results)
    {
        results.Clear();
        WIN32_FIND_DATAA data = {};

        char catPath[sizeof(Info::path)] = {};
        StrCpy(catPath, path);
        ReplaceChars(catPath, '\\', '/');
        RemoveChar(catPath, EndsWithChar(catPath, '/'));

        HANDLE h = ::FindFirstFileA(catPath, &data);
        if (!h || h == INVALID_HANDLE_VALUE)
        {
            return false;
        }

        RemoveAfter(catPath, StrRChr(catPath, '/'));

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
            info.createTime = CreateQWord(data.ftCreationTime.dwLowDateTime, data.ftCreationTime.dwHighDateTime);
            info.lastReadTime = CreateQWord(data.ftLastAccessTime.dwLowDateTime, data.ftLastAccessTime.dwHighDateTime);
            info.lastWriteTime = CreateQWord(data.ftLastWriteTime.dwLowDateTime, data.ftLastWriteTime.dwHighDateTime);
            info.size = CreateQWord(data.nFileSizeLow, data.nFileSizeHigh);

            StrCat(info.path, catPath);
            StrCat(info.path, data.cFileName);
            ToLower(info.path);

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
