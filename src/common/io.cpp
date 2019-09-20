
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

namespace IO
{
    static thread_local i32 ms_errno;

    i32 GetErrNo()
    {
        return ms_errno;
    }
    void SetErrNo(i32 value)
    {
        ms_errno = value;
        DebugInterrupt();
    }
    void ClearErrNo()
    {
        ms_errno = 0;
    }

    static i32 TestErrNo(i32 x)
    {
        if (x < 0)
        {
            SetErrNo(x);
        }
        return x;
    }

    // ------------------------------------------------------------------------

    FD Create(cstr filename)
    {
        return { TestErrNo(_creat(filename, _S_IREAD | _S_IWRITE)) };
    }
    FD Open(cstr filename, u32 flags)
    {
        return { TestErrNo(_open(filename, (i32)flags, _S_IREAD | _S_IWRITE)) };
    }
    bool Close(FD& hdl)
    {
        i32 fd = hdl.fd;
        hdl.fd = -1;
        // don't try to close invalid, stdin, stdout, or stderr
        if (fd > 2)
        {
            return !TestErrNo(_close(fd));
        }
        return false;
    }
    i32 Read(FD hdl, void* dst, usize size)
    {
        return TestErrNo(_read(hdl.fd, dst, (u32)size));
    }
    i32 Write(FD hdl, const void* src, usize size)
    {
        return TestErrNo(_write(hdl.fd, src, (u32)size));
    }
    i32 Seek(FD hdl, isize offset)
    {
        return TestErrNo((i32)_lseek(hdl.fd, (i32)offset, SEEK_SET));
    }
    i32 Tell(FD hdl)
    {
        return TestErrNo(_tell(hdl.fd));
    }
    bool Pipe(FD& p0, FD& p1, usize bufferSize)
    {
        i32 fds[2] = { -1, -1 };
        i32 rval = _pipe(fds, (u32)bufferSize, _O_BINARY);
        p0.fd = fds[0];
        p1.fd = fds[1];
        return !TestErrNo(rval);
    }
    bool Stat(FD hdl, Status& status)
    {
        status = {};
        return !TestErrNo(_fstat64(hdl.fd, (struct _stat64*)&status));
    }

    // ------------------------------------------------------------------------

    Writer::State Writer::Start(FD hdl, Slice<const u8> src)
    {
        Check(m_state == State::Idle, return SetErr());

        m_hdl = hdl;
        m_src = src;

        Check(IsOpen(hdl), return SetErr());
        Check(src.begin(), return SetErr());
        Check(src.size(), return SetErr());

        m_state = State::Writing;
        return m_state;
    }
    Writer::State Writer::Update()
    {
        Check(m_state == State::Writing, return SetErr());

        i32 wrote = Write(m_hdl, m_src.begin(), Min(WriteSize, m_src.size()));
        CheckE(return SetErr());

        isize rem = m_src.size() - wrote;
        m_src = { m_src.begin() + wrote, m_src.size() - wrote };

        if (rem <= 0)
        {
            Close(m_hdl);
            CheckE(return SetErr());
            m_state = State::Idle;
            m_src = { 0, 0 };
        }

        return m_state;
    }

    // ------------------------------------------------------------------------

    Reader::State Reader::Start(FD hdl)
    {
        Check(m_state == State::Idle, return SetErr());

        m_hdl = hdl;
        Check(IsOpen(hdl), return SetErr());

        m_state = State::Reading;
        return m_state;
    }
    Reader::State Reader::Update(Slice<u8>& result)
    {
        Check(m_state == State::Reading, return SetErr());

        result = { 0, 0 };
        i32 bytesRead = Read(m_hdl, m_dst, ReadSize);
        CheckE(return SetErr());

        if (bytesRead > 0)
        {
            result = { m_dst, (usize)bytesRead };
        }
        else
        {
            Close(m_hdl);
            CheckE(return SetErr());
            m_state = State::Idle;
        }

        return m_state;
    }

    // ------------------------------------------------------------------------

    Stream FOpen(cstr filename, cstr mode)
    {
        FILE* ptr = fopen(filename, mode);
        if (!ptr) { SetErrNo(-1); }
        return { ptr };
    }
    bool FClose(Stream& stream)
    {
        FILE* file = (FILE*)stream.ptr;
        stream.ptr = 0;
        if (file)
        {
            return !TestErrNo(fclose(file));
        }
        return false;
    }
    bool FFlush(Stream stream)
    {
        return !TestErrNo(fflush((FILE*)stream.ptr));
    }
    usize FRead(Stream stream, void* dst, usize size)
    {
        return fread(dst, 1, size, (FILE*)stream.ptr);
    }
    usize FWrite(Stream stream, const void* src, usize size)
    {
        return fwrite(src, 1, size, (FILE*)stream.ptr);
    }
    char* FGets(Stream stream, char* dst, usize size)
    {
        char* ptr = fgets(dst, (i32)(size - 1), (FILE*)stream.ptr);
        if (!ptr) { SetErrNo(-1); }
        return ptr;
    }
    i32 FPuts(Stream stream, cstr src)
    {
        return TestErrNo(fputs(src, (FILE*)stream.ptr));
    }
    FD FileNo(Stream stream)
    {
        return { TestErrNo(_fileno((FILE*)stream.ptr)) };
    }
    Stream FDOpen(FD& hdl, cstr mode)
    {
        FILE* ptr = _fdopen(hdl.fd, mode);
        if (ptr)
        {
            hdl.fd = -1;
        }
        else
        {
            SetErrNo(-1);
        }
        return { ptr };
    }
    bool FSeek(Stream stream, isize offset)
    {
        return !TestErrNo(fseek((FILE*)stream.ptr, (i32)offset, SEEK_SET));
    }
    i32 FTell(Stream stream)
    {
        return TestErrNo(ftell((FILE*)stream.ptr));
    }
    Stream POpen(cstr cmd, cstr mode)
    {
        FILE* file = _popen(cmd, mode);
        if (!file) { SetErrNo(-1); }
        return { file };
    }
    bool PClose(Stream& stream)
    {
        FILE* file = (FILE*)stream.ptr;
        stream.ptr = 0;
        if (file)
        {
            return !TestErrNo(_pclose(file));
        }
        return false;
    }

    // ------------------------------------------------------------------------

    bool ChDrive(i32 drive)
    {
        return !TestErrNo(_chdrive(drive));
    }
    i32 GetDrive()
    {
        return TestErrNo(_getdrive());
    }
    u32 GetDrivesMask()
    {
        return _getdrives();
    }
    bool GetCwd(char* dst, i32 size)
    {
        Check0(dst);
        char* ptr = _getcwd(dst, size);
        if (!ptr) { SetErrNo(-1); }
        return ptr != 0;
    }
    bool GetDrCwd(i32 drive, char* dst, i32 size)
    {
        Check0(dst);
        char* ptr = _getdcwd(drive, dst, size);
        if (!ptr) { SetErrNo(-1); }
        return ptr != 0;
    }
    bool ChDir(cstr path)
    {
        Check0(path);
        return !_chdir(path);
    }
    bool MkDir(cstr path)
    {
        Check0(path);
        return !_mkdir(path);
    }
    bool RmDir(cstr path)
    {
        Check0(path);
        return !_rmdir(path);
    }
    bool ChMod(cstr path, u32 flags)
    {
        Check0(path);
        return !TestErrNo(_chmod(path, (i32)flags));
    }

    // ------------------------------------------------------------------------

    bool SearchEnv(cstr filename, cstr varname, char(&dst)[260])
    {
        Check0(filename);
        Check0(varname);
        dst[0] = 0;
        _searchenv(filename, varname, dst);
        bool found = dst[0] != 0;
        if (!found) { SetErrNo(-1); }
        return found;
    }
    cstr GetEnv(cstr varname)
    {
        Check0(varname);
        cstr ptr = getenv(varname);
        if (!ptr) { SetErrNo(-1); }
        return ptr;
    }
    bool PutEnv(cstr varname, cstr value)
    {
        Check0(varname);
        Check0(value);
        char buf[260];
        i32 rval = sprintf_s(buf, "%s=%s", varname, value ? value : "");
        TestErrNo(rval);
        return !TestErrNo(_putenv(buf));
    }

    // ------------------------------------------------------------------------

    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/findfirst-functions
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/findnext-functions
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/findclose

    bool Find(Finder& fdr, FindData& data, cstrc spec)
    {
        switch (fdr.state)
        {
        case 0:
            fdr.hdl = _findfirst64(spec, (struct __finddata64_t*)&data);
            fdr.state = IsOpen(fdr) ? 1 : 0;
            return IsOpen(fdr);
        case 1:
            if (!_findnext64(fdr.hdl, (struct __finddata64_t*)&data))
            {
                return true;
            }
            TestErrNo(_findclose(fdr.hdl));
            fdr.hdl = -1;
            fdr.state = 0;
            return false;
        }
        fdr.hdl = -1;
        fdr.state = 0;
        return false;
    }

    void FindAll(Array<FindData>& results, cstrc spec)
    {
        Finder fdr = {};
        while (Find(fdr, results.grow(), spec)) {};
        results.pop();
    }

    // ------------------------------------------------------------------------

    bool OpenModule(cstr filename, Module& dst)
    {
        Check0(filename);
        Check0(!dst.hdl);
        dst.hdl = 0;
        dst.hdl = ::LoadLibraryA(filename);
        return dst.hdl != 0;
    }

    bool CloseModule(Module& mod)
    {
        Check0(mod.hdl);
        void* hdl = mod.hdl;
        mod.hdl = 0;
        return ::FreeLibrary((HMODULE)hdl);
    }

    bool GetModule(cstr name, Module& dst)
    {
        Check0(!dst.hdl);
        dst.hdl = 0;
        dst.hdl = ::GetModuleHandleA(name);
        return dst.hdl != 0;
    }

    bool GetModuleName(Module mod, char* dst, u32 dstSize)
    {
        Check0(dst);
        Check0(dstSize);
        u32 wrote = ::GetModuleFileNameA((HMODULE)mod.hdl, dst, dstSize);
        return wrote != 0;
    }

    void* GetModuleFunc(Module mod, cstr name)
    {
        Check0(mod.hdl);
        Check0(name);
        FARPROC proc = ::GetProcAddress((HMODULE)mod.hdl, name);
        return (void*)proc;
    }

    // ------------------------------------------------------------------------
}; // IO
