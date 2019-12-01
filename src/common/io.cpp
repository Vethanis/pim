
#define _CRT_SECURE_NO_WARNINGS 1
#define WIN32_LEAN_AND_MEAN 1

#include <windows.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <direct.h>

#include "common/io.h"
#include "common/macro.h"
#include "common/stringutil.h"
#include <string.h>

namespace IO
{
    static i32 NotNeg(i32 x, EResult& err)
    {
        err = x < 0 ? EFail : ESuccess;
        return x;
    }

    static void* NotNull(void* x, EResult& err)
    {
        err = x ? ESuccess : EFail;
        return x;
    }

    static i32 IsZero(i32 x, EResult& err)
    {
        err = x ? ESuccess : EFail;
        return x;
    }

    // ------------------------------------------------------------------------

    FD Create(cstr filename, EResult& err)
    {
        Assert(filename);
        return { NotNeg(_creat(filename, _S_IREAD | _S_IWRITE), err) };
    }
    FD Open(cstr filename, u32 flags, EResult& err)
    {
        Assert(filename);
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
        Assert(IsOpen(hdl));
        Assert(dst);
        Assert(size);
        return NotNeg(_read(hdl.fd, dst, (u32)size), err);
    }
    i32 Write(FD hdl, const void* src, usize size, EResult& err)
    {
        Assert(IsOpen(hdl));
        Assert(src);
        Assert(size);
        return NotNeg(_write(hdl.fd, src, (u32)size), err);
    }
    i32 Puts(cstrc str, FD hdl)
    {
        Assert(str);
        Assert(IsOpen(hdl));
        EResult err = EUnknown;
        i32 ct = Write(hdl, str, StrLen(str), err);
        Assert(err == ESuccess);
        return ct;
    }
    i32 Seek(FD hdl, isize offset, EResult& err)
    {
        Assert(IsOpen(hdl));
        return NotNeg((i32)_lseek(hdl.fd, (i32)offset, SEEK_SET), err);
    }
    i32 Tell(FD hdl, EResult& err)
    {
        Assert(IsOpen(hdl));
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
        Assert(IsOpen(hdl));
        status = {};
        IsZero(_fstat64(hdl.fd, (struct _stat64*)&status), err);
    }

    // ------------------------------------------------------------------------

    Stream FOpen(cstr filename, cstr mode, EResult& err)
    {
        Assert(filename);
        Assert(mode);
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
        Assert(IsOpen(stream));
        IsZero(fflush((FILE*)stream.ptr), err);
    }
    usize FRead(Stream stream, void* dst, usize size, EResult& err)
    {
        Assert(IsOpen(stream));
        Assert(dst);
        Assert(size);
        usize ct = fread(dst, 1, size, (FILE*)stream.ptr);
        err = ct == size ? ESuccess : EFail;
        return ct;
    }
    usize FWrite(Stream stream, const void* src, usize size, EResult& err)
    {
        Assert(IsOpen(stream));
        Assert(src);
        Assert(size);
        usize ct = fwrite(src, 1, size, (FILE*)stream.ptr);
        err = ct == size ? ESuccess : EFail;
        return ct;
    }
    void FGets(Stream stream, char* dst, usize size, EResult& err)
    {
        Assert(IsOpen(stream));
        Assert(dst);
        Assert(size > 0);
        NotNull(fgets(dst, (i32)(size - 1), (FILE*)stream.ptr), err);
    }
    i32 FPuts(Stream stream, cstr src, EResult& err)
    {
        Assert(IsOpen(stream));
        Assert(src);
        return NotNeg(fputs(src, (FILE*)stream.ptr), err);
    }
    FD FileNo(Stream stream, EResult& err)
    {
        Assert(IsOpen(stream));
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
        Assert(IsOpen(stream));
        NotNeg(fseek((FILE*)stream.ptr, (i32)offset, SEEK_SET), err);
    }
    i32 FTell(Stream stream, EResult& err)
    {
        Assert(IsOpen(stream));
        return NotNeg(ftell((FILE*)stream.ptr), err);
    }
    Stream POpen(cstr cmd, cstr mode, EResult& err)
    {
        Assert(cmd);
        Assert(mode);
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
        Assert(dst);
        Assert(size > 0);
        NotNull(_getcwd(dst, size), err);
    }
    void GetDrCwd(i32 drive, char* dst, i32 size, EResult& err)
    {
        Assert(dst);
        Assert(size > 0);
        NotNull(_getdcwd(drive, dst, size), err);
    }
    void ChDir(cstr path, EResult& err)
    {
        Assert(path);
        IsZero(_chdir(path), err);
    }
    void MkDir(cstr path, EResult& err)
    {
        Assert(path);
        IsZero(_mkdir(path), err);
    }
    void RmDir(cstr path, EResult& err)
    {
        Assert(path);
        IsZero(_rmdir(path), err);
    }
    void ChMod(cstr path, u32 flags, EResult& err)
    {
        Assert(path);
        IsZero(_chmod(path, (i32)flags), err);
    }

    // ------------------------------------------------------------------------

    void SearchEnv(cstr filename, cstr varname, char(&dst)[260], EResult& err)
    {
        Assert(filename);
        Assert(varname);
        dst[0] = 0;
        _searchenv(filename, varname, dst);
        err = dst[0] != 0 ? ESuccess : EFail;
    }
    cstr GetEnv(cstr varname, EResult& err)
    {
        Assert(varname);
        return (cstr)NotNull(getenv(varname), err);
    }
    void PutEnv(cstr varname, cstr value, EResult& err)
    {
        Assert(varname);
        char buf[260];
        SPrintf(argof(buf), "%s=%s", varname, value ? value : "");
        IsZero(_putenv(buf), err);
    }

    // ------------------------------------------------------------------------

    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/findfirst-functions
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/findnext-functions
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/findclose

    bool Find(Finder& fdr, FindData& data, cstrc spec, EResult& err)
    {
        switch (fdr.state)
        {
        case 0:
            Assert(spec);
            fdr.hdl = _findfirst64(spec, (struct __finddata64_t*)&data);
            if (IsOpen(fdr))
            {
                fdr.state = 1;
                err = ESuccess;
                return true;
            }
            fdr.state = 0;
            err = EFail;
            return false;
        case 1:
            Assert(IsOpen(fdr));
            if (!_findnext64(fdr.hdl, (struct __finddata64_t*)&data))
            {
                return true;
            }
            _findclose(fdr.hdl);
            fdr.hdl = -1;
            fdr.state = 0;
            return false;
        default:
            DebugInterrupt();
            err = EFail;
            fdr.hdl = -1;
            fdr.state = 0;
            return false;
        }
    }

    void FindAll(Array<FindData>& results, cstrc spec, EResult& err)
    {
        Finder fdr = {};
        while (Find(fdr, results.Grow(), spec, err)) {};
        results.Pop();
    }

    // ------------------------------------------------------------------------

    void OpenModule(cstr filename, Module& dst, EResult& err)
    {
        Assert(filename);
        Assert(!dst.hdl);
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
        Assert(!dst.hdl);
        dst.hdl = NotNull(::GetModuleHandleA(name), err);
    }

    void GetModuleName(Module mod, char* dst, u32 dstSize, EResult& err)
    {
        Assert(dst);
        Assert(dstSize);
        dst[0] = 0;
        u32 wrote = ::GetModuleFileNameA((HMODULE)mod.hdl, dst, dstSize);
        err = wrote != 0 ? ESuccess : EFail;
    }

    void GetModuleFunc(Module mod, cstr name, void** pFunc, EResult& err)
    {
        Assert(name);
        Assert(pFunc);
        *pFunc = NotNull(::GetProcAddress((HMODULE)mod.hdl, name), err);
    }

    // ------------------------------------------------------------------------

    void Curl(cstr url, Array<char>& result, EResult& err)
    {
        Assert(url);
        result.Clear();
        char cmd[1024];
        SPrintf(
            argof(cmd),
            "tools\\curl\\curl.exe --cacert tools\\curl\\ssl\\cert.pem -L -B %s",
            url);
        Stream stream = POpen(cmd, "rb", err);
        if (err == ESuccess)
        {
            while (true)
            {
                i32 bytesRead = (i32)FRead(stream, argof(cmd), err);
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
}; // IO
