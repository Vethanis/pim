
#define _CRT_SECURE_NO_WARNINGS 1
#include "common/macro.h"

#include <windows.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <direct.h>

#include "common/io.h"
#include "common/stringutil.h"

namespace IO
{
    static i32 NotNeg(i32 x, EResult& err)
    {
        err = (x >= 0) ? ESuccess : EFail;
        return x;
    }

    static void* NotNull(void* x, EResult& err)
    {
        err = (x != nullptr) ? ESuccess : EFail;
        return x;
    }

    static i32 IsZero(i32 x, EResult& err)
    {
        err = (x == 0) ? ESuccess : EFail;
        return x;
    }

    // ------------------------------------------------------------------------

    FD Create(cstr filename, u32 flags, EResult& err)
    {
        ASSERT(filename);

        i32 pMode = _S_IREAD;
        if (flags & OReadWrite)
        {
            pMode |= _S_IWRITE;
        }
        i32 oFlags = (i32)flags | _O_CREAT | _O_EXCL;

        i32 fd = _open(
            filename,
            oFlags,
            pMode);

        return { NotNeg(fd, err) };
    }

    FD Open(cstr filename, u32 flags, EResult& err)
    {
        ASSERT(filename);
        return { NotNeg(_open(filename, (i32)flags, _S_IREAD | _S_IWRITE), err) };
    }

    void Close(FD& hdl, EResult& err)
    {
        i32 fd = hdl.fd;
        hdl.fd = -1;
        // don't try to close invalid, stdin, stdout, or stderr
        if (fd > 2)
        {
            IsZero(_close(fd), err);
        }
    }

    i32 Read(FD hdl, void* dst, usize size, EResult& err)
    {
        ASSERT(IsOpen(hdl));
        ASSERT(dst);
        ASSERT(size);
        return NotNeg(_read(hdl.fd, dst, (u32)size), err);
    }

    i32 Write(FD hdl, const void* src, usize size, EResult& err)
    {
        ASSERT(IsOpen(hdl));
        ASSERT(src);
        ASSERT(size);
        return NotNeg(_write(hdl.fd, src, (u32)size), err);
    }

    void Grow(FD hdl, usize size, EResult& err)
    {
        ASSERT(IsOpen(hdl));
        if (size > 0)
        {
            i64 curSize = Size(hdl, err);
            if (size > (usize)curSize)
            {
                i32 prevOffset = Tell(hdl, err);
                Seek(hdl, size - 1, err);
                u8 x = 0;
                Write(hdl, &x, 1, err);
                Seek(hdl, prevOffset, err);
            }
        }
    }

    i32 Puts(cstrc str, FD hdl)
    {
        ASSERT(str);
        ASSERT(IsOpen(hdl));
        EResult err = EUnknown;
        i32 ct = Write(hdl, str, StrLen(str), err);
        ASSERT(err == ESuccess);
        ct += Write(hdl, "\n", 1, err);
        ASSERT(err == ESuccess);
        return ct;
    }

    i32 Printf(FD hdl, cstr fmt, ...)
    {
        ASSERT(fmt);
        ASSERT(IsOpen(hdl));
        char buffer[256] = {};
        const i32 ct = VSPrintf(ARGS(buffer), fmt, VaStart(fmt));
        EResult err = EUnknown;
        Write(hdl, buffer, ct, err);
        ASSERT(err == ESuccess);
        return ct;
    }

    i32 Seek(FD hdl, isize offset, EResult& err)
    {
        ASSERT(IsOpen(hdl));
        return NotNeg((i32)_lseek(hdl.fd, (i32)offset, SEEK_SET), err);
    }

    i32 Tell(FD hdl, EResult& err)
    {
        ASSERT(IsOpen(hdl));
        return NotNeg(_tell(hdl.fd), err);
    }

    void Pipe(FD& p0, FD& p1, usize bufferSize, EResult& err)
    {
        i32 fds[2] = { -1, -1 };
        IsZero(_pipe(fds, (u32)bufferSize, _O_BINARY), err);
        p0.fd = fds[0];
        p1.fd = fds[1];
    }

    void Stat(FD hdl, Status& status, EResult& err)
    {
        ASSERT(IsOpen(hdl));
        status = {};
        IsZero(_fstat64(hdl.fd, (struct _stat64*)&status), err);
    }

    // ------------------------------------------------------------------------

    Stream FOpen(cstr filename, cstr mode, EResult& err)
    {
        ASSERT(filename);
        ASSERT(mode);
        return { NotNull(fopen(filename, mode), err) };
    }
    void FClose(Stream& stream, EResult& err)
    {
        FILE* file = (FILE*)stream.ptr;
        stream.ptr = 0;
        if (file)
        {
            IsZero(fclose(file), err);
        }
    }
    void FFlush(Stream stream, EResult& err)
    {
        ASSERT(IsOpen(stream));
        IsZero(fflush((FILE*)stream.ptr), err);
    }
    usize FRead(Stream stream, void* dst, usize size, EResult& err)
    {
        ASSERT(IsOpen(stream));
        ASSERT(dst);
        ASSERT(size);
        usize ct = fread(dst, 1, size, (FILE*)stream.ptr);
        err = ct == size ? ESuccess : EFail;
        return ct;
    }
    usize FWrite(Stream stream, const void* src, usize size, EResult& err)
    {
        ASSERT(IsOpen(stream));
        ASSERT(src);
        ASSERT(size);
        usize ct = fwrite(src, 1, size, (FILE*)stream.ptr);
        err = ct == size ? ESuccess : EFail;
        return ct;
    }
    void FGets(Stream stream, char* dst, usize size, EResult& err)
    {
        ASSERT(IsOpen(stream));
        ASSERT(dst);
        ASSERT(size > 0);
        NotNull(fgets(dst, (i32)(size - 1), (FILE*)stream.ptr), err);
    }
    i32 FPuts(Stream stream, cstr src, EResult& err)
    {
        ASSERT(IsOpen(stream));
        ASSERT(src);
        return NotNeg(fputs(src, (FILE*)stream.ptr), err);
    }
    FD FileNo(Stream stream, EResult& err)
    {
        ASSERT(IsOpen(stream));
        return { NotNeg(_fileno((FILE*)stream.ptr), err) };
    }
    Stream FDOpen(FD& hdl, cstr mode, EResult& err)
    {
        FILE* ptr = _fdopen(hdl.fd, mode);
        if (ptr)
        {
            hdl.fd = -1;
        }
        return { NotNull(ptr, err) };
    }
    void FSeek(Stream stream, isize offset, EResult& err)
    {
        ASSERT(IsOpen(stream));
        NotNeg(fseek((FILE*)stream.ptr, (i32)offset, SEEK_SET), err);
    }
    i32 FTell(Stream stream, EResult& err)
    {
        ASSERT(IsOpen(stream));
        return NotNeg(ftell((FILE*)stream.ptr), err);
    }
    Stream POpen(cstr cmd, cstr mode, EResult& err)
    {
        ASSERT(cmd);
        ASSERT(mode);
        FILE* file = _popen(cmd, mode);
        return { NotNull(file, err) };
    }
    void PClose(Stream& stream, EResult& err)
    {
        FILE* file = (FILE*)stream.ptr;
        stream.ptr = 0;
        if (file)
        {
            IsZero(_pclose(file), err);
        }
    }

    // ------------------------------------------------------------------------

    void ChDrive(i32 drive, EResult& err)
    {
        IsZero(_chdrive(drive), err);
    }
    i32 GetDrive(EResult& err)
    {
        return IsZero(_getdrive(), err);
    }
    u32 GetDrivesMask(EResult& err)
    {
        u32 mask = _getdrives();
        err = mask ? ESuccess : EFail;
        return mask;
    }
    void GetCwd(char* dst, i32 size, EResult& err)
    {
        ASSERT(dst);
        ASSERT(size > 0);
        NotNull(_getcwd(dst, size), err);
    }
    void GetDrCwd(i32 drive, char* dst, i32 size, EResult& err)
    {
        ASSERT(dst);
        ASSERT(size > 0);
        NotNull(_getdcwd(drive, dst, size), err);
    }
    void ChDir(cstr path, EResult& err)
    {
        ASSERT(path);
        IsZero(_chdir(path), err);
    }
    void MkDir(cstr path, EResult& err)
    {
        ASSERT(path);
        IsZero(_mkdir(path), err);
    }
    void RmDir(cstr path, EResult& err)
    {
        ASSERT(path);
        IsZero(_rmdir(path), err);
    }
    void ChMod(cstr path, u32 flags, EResult& err)
    {
        ASSERT(path);
        IsZero(_chmod(path, (i32)flags), err);
    }

    // ------------------------------------------------------------------------

    void SearchEnv(cstr filename, cstr varname, char(&dst)[260], EResult& err)
    {
        ASSERT(filename);
        ASSERT(varname);
        dst[0] = 0;
        _searchenv(filename, varname, dst);
        err = dst[0] != 0 ? ESuccess : EFail;
    }
    cstr GetEnv(cstr varname, EResult& err)
    {
        ASSERT(varname);
        return (cstr)NotNull(getenv(varname), err);
    }
    void PutEnv(cstr varname, cstr value, EResult& err)
    {
        ASSERT(varname);
        char buf[260];
        SPrintf(ARGS(buf), "%s=%s", varname, value ? value : "");
        IsZero(_putenv(buf), err);
    }

    // ------------------------------------------------------------------------

    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/findfirst-functions
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/findnext-functions
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/findclose

    bool Begin(Finder& fdr, FindData& data, cstrc spec)
    {
        ASSERT(spec);
        ASSERT(!IsOpen(fdr));
        fdr.hdl = _findfirst64(spec, (struct __finddata64_t*)&data);
        return IsOpen(fdr);
    }

    bool Next(Finder& fdr, FindData& data)
    {
        if (IsOpen(fdr))
        {
            if (!_findnext64(fdr.hdl, (struct __finddata64_t*)&data))
            {
                return true;
            }
        }
        return false;
    }

    void Close(Finder& fdr)
    {
        if (IsOpen(fdr))
        {
            _findclose(fdr.hdl);
            fdr.hdl = -1;
        }
    }

    bool Find(Finder& fdr, FindData& data, cstrc spec)
    {
        if (IsOpen(fdr))
        {
            if (Next(fdr, data))
            {
                return true;
            }
            Close(fdr);
            return false;
        }
        else
        {
            return Begin(fdr, data, spec);
        }
    }

    void FindAll(Array<FindData>& results, cstrc spec)
    {
        results.Clear();
        Finder fdr = {};
        FindData data = {};
        while (Find(fdr, data, spec))
        {
            results.Grow() = data;
        }
    }

    void ListDir(Directory& dir, cstrc spec)
    {
        constexpr u32 IgnoreFlags = FAF_Hidden | FAF_System;

        Finder fdr = {};
        FindData data = {};
        while (Find(fdr, data, spec))
        {
            if (data.attrib & IgnoreFlags)
            {
                continue;
            }

            if (data.attrib & FAF_SubDir)
            {
                if (!StrCmp(ARGS(data.name), ".") ||
                    !StrCmp(ARGS(data.name), ".."))
                {
                    continue;
                }

                Directory& subdir = dir.subdirs.Grow();
                SPrintf(
                    ARGS(subdir.data.name),
                    "%s\\%s",
                    dir.data.name,
                    data.name);

                subdir.subdirs.Init(dir.subdirs.GetAllocator());
                subdir.files.Init(dir.subdirs.GetAllocator());

                char buffer[PIM_PATH] = {};
                {
                    StrCpy(ARGS(buffer), spec);
                    char* pStar = (char*)StrStr(ARGS(buffer), "*");
                    if (pStar)
                    {
                        *pStar = 0;
                    }
                    StrCat(ARGS(buffer), data.name);
                    StrCat(ARGS(buffer), "\\*");
                }

                ListDir(subdir, buffer);
            }
            else
            {
                FindData& file = dir.files.Grow();
                file = data;
                StrCpy(ARGS(file.name), dir.data.name);
                StrCat(ARGS(file.name), "\\");
                StrCat(ARGS(file.name), data.name);
            }
        }
    }

    // ------------------------------------------------------------------------

    void OpenModule(cstr filename, Module& dst, EResult& err)
    {
        ASSERT(filename);
        ASSERT(!dst.hdl);
        dst.hdl = NotNull(::LoadLibraryA(filename), err);
    }

    void CloseModule(Module& mod, EResult& err)
    {
        void* hdl = mod.hdl;
        mod.hdl = 0;
        if (hdl)
        {
            bool freed = ::FreeLibrary((HMODULE)hdl);
            err = freed ? ESuccess : EFail;
        }
    }

    void GetModule(cstr name, Module& dst, EResult& err)
    {
        ASSERT(!dst.hdl);
        dst.hdl = NotNull(::GetModuleHandleA(name), err);
    }

    void GetModuleName(Module mod, char* dst, u32 dstSize, EResult& err)
    {
        ASSERT(dst);
        ASSERT(dstSize);
        dst[0] = 0;
        u32 wrote = ::GetModuleFileNameA((HMODULE)mod.hdl, dst, dstSize);
        err = wrote != 0 ? ESuccess : EFail;
    }

    void GetModuleFunc(Module mod, cstr name, void** pFunc, EResult& err)
    {
        ASSERT(name);
        ASSERT(pFunc);
        *pFunc = NotNull(::GetProcAddress((HMODULE)mod.hdl, name), err);
    }

    // ------------------------------------------------------------------------

    void Curl(cstr url, Array<char>& result, EResult& err)
    {
        ASSERT(url);
        result.Clear();
        char cmd[1024];
        SPrintf(
            ARGS(cmd),
            "tools\\curl\\curl.exe --cacert tools\\curl\\ssl\\cert.pem -L -B %s",
            url);
        Stream stream = POpen(cmd, "rb", err);
        if (err == ESuccess)
        {
            while (true)
            {
                i32 bytesRead = (i32)FRead(stream, ARGS(cmd), err);
                if (bytesRead == 0)
                {
                    break;
                }
                i32 prevSize = result.ResizeRel(bytesRead);
                memcpy(result.begin() + prevSize, cmd, bytesRead);
            }
        }
        PClose(stream, err);
        err = result.IsEmpty() ? EFail : ESuccess;
    }

    // ------------------------------------------------------------------------

    FileMap MapFile(IO::FD fd, bool writable, EResult& err)
    {
        err = EUnknown;

        if (!IsOpen(fd))
        {
            err = EFail;
            return {};
        }

        const i32 fileSize = (i32)Size(fd, err);
        if (err == EFail)
        {
            return {};
        }

        if (fileSize <= 0)
        {
            err = EFail;
            return {};
        }

        HANDLE hFile = (HANDLE)_get_osfhandle(fd.fd);
        if (hFile == INVALID_HANDLE_VALUE)
        {
            err = EFail;
            return {};
        }

        const u32 flProtect =
            writable ? PAGE_READWRITE : PAGE_READONLY;

        HANDLE hMapping = CreateFileMappingA(
            hFile,
            nullptr,
            flProtect,
            0u,
            0u,
            nullptr);
        if (hMapping == INVALID_HANDLE_VALUE)
        {
            err = EFail;
            return {};
        }

        constexpr u32 FILE_MAP_READWRITE = FILE_MAP_READ | FILE_MAP_WRITE;

        const u32 viewAccess =
            writable ? FILE_MAP_READWRITE : FILE_MAP_READ;

        void* addr = MapViewOfFile(
            hMapping,
            viewAccess,
            0u,
            0u,
            0u);
        if (!addr)
        {
            CloseHandle(hMapping);
            err = EFail;
            return {};
        }

        FileMap map = {};
        map.fd = fd;
        map.hMapping = hMapping;
        map.memory = { (u8*)addr, fileSize };

        err = ESuccess;

        return map;
    }

    void Close(FileMap& map)
    {
        if (IsOpen(map))
        {
            UnmapViewOfFile(map.memory.begin());
            CloseHandle(map.hMapping);
            EResult err;
            Close(map.fd, err);
        }

        map = {};
    }

    bool Flush(
        FileMap map,
        i32 offset,
        i32 size)
    {
        Slice<u8> region = map.memory.Subslice(offset, size);
        return FlushViewOfFile(region.begin(), region.size());
    }
};
