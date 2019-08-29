
#include "common/platform.h"

#if PLAT_WINDOWS

#include "common/fs.h"
#include "common/memory.h"
#include "common/macro.h"
#include "common/stringutil.h"
#include "common/thread.h"
#include "common/random.h"

#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/stat.h>

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
    static char ms_tempPath[FS_MAX_PATH];
    static bool ms_hasTempPath;

    static Stream* ToStream(FILE* pFile)
    {
        return (Stream*)pFile;
    }
    static FILE* ToFile(Stream* pStream)
    {
        return (FILE*)pStream;
    }
    static FILE* ToFile(const Stream* pStream)
    {
        return (FILE*)pStream;
    }

    static void ModeToStr(char* ptr, u32 mode)
    {
        bool overwrite = mode & SM_Overwrite;
        bool r = mode & SM_Read;
        bool w = mode & SM_Write;
        bool c = mode & SM_Commit;
        bool n = mode & SM_NoCommit;
        bool S = mode & SM_Sequential;
        bool R = mode & SM_Random;
        bool T = mode & SM_Temporary;
        bool D = mode & SM_Delete;

        DebugAssert(r || w);
        DebugAssert(!(c && n));
        DebugAssert(!(S && R));

        if (r & w)
        {
            if (overwrite)
            {
                *ptr++ = 'w';
            }
            else
            {
                *ptr++ = 'r';
            }
            *ptr++ = '+';
        }
        else if (r)
        {
            *ptr++ = 'r';
        }
        else if (w)
        {
            *ptr++ = 'w';
        }

        if (c)
        {
            *ptr++ = 'c';
        }
        else if (n)
        {
            *ptr++ = 'n';
        }

        if (S)
        {
            *ptr++ = 'S';
        }
        else if (R)
        {
            *ptr++ = 'R';
        }

        if (T)
        {
            *ptr++ = 'T';
        }
        else if (D)
        {
            *ptr++ = 'D';
        }

        *ptr++ = 'b';
        *ptr++ = 0;
    }

    Stream* Stream::OpenTemporary(u32 mode)
    {
        mode |= SM_Temporary;
        mode |= SM_Delete;
        mode &= ~SM_Commit;

        if (!ms_hasTempPath)
        {
            ms_hasTempPath = true;
            u32 rval = GetTempPathA(sizeof(ms_tempPath), ms_tempPath);
            DebugAssert(rval);
        }

        char tempName[FS_MAX_PATH] = {};
        StrCat(tempName, ms_tempPath);
        {
            char tempId[64] = {};
            u32 threadId = Thread::Self().id;
            sprintf_s(tempId, "/PIM-%x-%x.tmp", threadId, Random::NextU32());
            StrCat(tempName, tempId);
        }

        return Open(tempName, mode);
    }
    Stream* Stream::Open(cstr path, u32 mode)
    {
        char modeStr[16];
        ModeToStr(modeStr, mode);
        FILE* pFile = 0;
        errno_t err = fopen_s(&pFile, path, modeStr);
        if (err)
        {
            DebugAssert(!pFile);
            return 0;
        }
        return ToStream(pFile);
    }
    Stream* Stream::OpenBuffered(
        cstr path,
        u32 mode,
        void* buffer,
        usize size)
    {
        Stream* stream = Open(path, mode);
        if (stream)
        {
            setvbuf(ToFile(stream), (char*)buffer, _IOFBF, size);
        }
    }
    void Stream::Close(Stream* stream)
    {
        if (stream)
        {
            _fclose_nolock(ToFile(stream));
        }
    }
    void Stream::CloseAll()
    {
        _fcloseall();
    }

    void Stream::Flush()
    {
        _fflush_nolock(ToFile(this));
    }
    void Stream::Lock()
    {
        _lock_file(ToFile(this));
    }
    void Stream::Unlock()
    {
        _unlock_file(ToFile(this));
    }

    i32 Stream::GetFileNumber() const
    {
        return _fileno(ToFile(this));
    }
    i32 Stream::GetError() const
    {
        return ferror(ToFile(this));
    }
    void Stream::ClearError()
    {
        clearerr_s(ToFile(this));
    }

    isize Stream::Size() const
    {
        struct _stat64 s = {};
        _fstat64(GetFileNumber(), &s);
        return s.st_size;
    }
    isize Stream::Tell() const
    {
        return _ftelli64_nolock(ToFile(this));
    }
    void Stream::Seek(isize offset)
    {
        _fseeki64_nolock(ToFile(this), offset, SEEK_SET);
    }
    bool Stream::AtEnd() const
    {
        return feof(ToFile(this));
    }

    i32 Stream::GetChar()
    {
        return _fgetc_nolock(ToFile(this));
    }
    void Stream::PutChar(i32 c)
    {
        _fputc_nolock(c, ToFile(this));
    }
    void Stream::PutStr(cstr src)
    {
        if (src)
        {
            usize len = StrLen(src);
            FILE* pFile = ToFile(this);
            _fwrite_nolock(src, 1u, len, pFile);
            _fputc_nolock('\n', pFile);
        }
    }
    void Stream::Print(cstr fmt, ...)
    {
        va_list lst;
        va_start(lst, fmt);
        vfprintf_s(ToFile(this), fmt, lst);
        va_end(lst);
    }
    void Stream::Scan(cstr fmt, ...)
    {
        va_list lst;
        va_start(lst, fmt);
        vfscanf_s(ToFile(this), fmt, lst);
        va_end(lst);
    }

    void Stream::_Write(const void* src, usize len)
    {
        _fwrite_nolock(src, 1, len, ToFile(this));
    }
    void Stream::_Read(void* dst, usize len)
    {
        _fread_nolock_s(dst, len, 1, len, ToFile(this));
    }
};

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
