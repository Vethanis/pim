#pragma once

#include "common/macro.h"
#include "common/int_types.h"
#include "containers/slice.h"
#include "containers/array.h"

enum EResult
{
    EUnknown = 0,
    ESuccess = 1,
    EFail = 2,
};

namespace IO
{
    // file descriptor
    struct FD { i32 fd; };
    inline bool IsOpen(FD hdl) { return hdl.fd >= 0; }

    static constexpr FD StdIn  = { 0 };
    static constexpr FD StdOut = { 1 };
    static constexpr FD StdErr = { 2 };

    enum StModeFlags : u16
    {
        StMode_Regular  = 0x8000,
        StMode_Dir      = 0x4000,
        StMode_SpecChar = 0x2000,
        StMode_Pipe     = 0x1000,
        StMode_Read     = 0x0100,
        StMode_Write    = 0x0080,
        StMode_Exec     = 0x0040,
    };

    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/fstat-fstat32-fstat64-fstati64-fstat32i64-fstat64i32
    struct Status
    {
        u32 st_dev;
        u16 st_ino;
        u16 st_mode;
        i16 st_nlink;
        i16 st_uid;
        i16 st_guid;
        u32 st_rdev;
        i64 st_size;
        i64 st_atime;
        i64 st_mtime;
        i64 st_ctime;
    };

    // derived from values in wdk/include/10.0.17763.0/ucrt/fcntl.h
    // file control options used by _open()
    enum OFlags : u32
    {
        OReadOnly   = 0x0000,   // open for reading only
        OWriteOnly  = 0x0001,   // open for writing only
        OReadWrite  = 0x0002,   // open for reading and writing
        OAppend     = 0x0008,   // writes done at eof

        ORandom     = 0x0010,   // optimize for random accesses
        OSequential = 0x0020,   // optimize for sequential accesses (preferred)
        OTemporary  = 0x0040,   // file is deleted when last handle is closed (ref counted)
        ONoInherit  = 0x0080,   // child process doesn't inherit file (private fd)

        OCreate     = 0x0100,   // create and open file
        OTruncate   = 0x0200,   // open and truncate
        OExclusive  = 0x0400,   // open only if file doesnt exist

        OShortLived = 0x1000,   // temporary storage, avoid flushing
        OObtainDir  = 0x2000,   // get directory info
        OText       = 0x4000,   // file mode is text (translated, crlf -> lf on read, lf -> crlf on write)
        OBinary     = 0x8000,   // file mode is binary (untranslated, preferred)

        OBinSeqRead     = OBinary | OSequential | OReadOnly,
        OBinSeqWrite    = OBinary | OSequential | OWriteOnly | OCreate,
        OBinRandRW      = OBinary | ORandom | OReadWrite,
    };

    // creat
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/creat-wcreat
    FD Create(cstr filename, EResult& err);

    // open
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/open-wopen
    FD Open(cstr filename, u32 flags, EResult& err);

    // close
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/close
    void Close(FD& hdl, EResult& err);

    // read
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/read
    i32 Read(FD hdl, void* dst, usize size, EResult& err);

    // write
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/write
    i32 Write(FD hdl, const void* src, usize size, EResult& err);

    i32 Puts(cstrc str, FD hdl = StdOut);

    // seek
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/lseek-lseeki64
    i32 Seek(FD hdl, isize offset, EResult& err);

    // tell
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/tell-telli64
    i32 Tell(FD hdl, EResult& err);

    // pipe
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/pipe
    void Pipe(FD& p0, FD& p1, usize bufferSize, EResult& err);

    // fstat
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/fstat-fstat32-fstat64-fstati64-fstat32i64-fstat64i32
    void Stat(FD hdl, Status& status, EResult& err);

    // fstat.st_size
    inline i64 Size(FD fd, EResult& err)
    {
        Status status = {};
        Stat(fd, status, err);
        return status.st_size;
    }

    struct FDGuard
    {
        FD m_fd;
        EResult m_result;

        FDGuard(FD fd) : m_fd(fd), m_result(EUnknown) {}
        ~FDGuard() { Close(m_fd, m_result); }

        inline operator FD () const
        {
            return m_fd;
        }

        inline FD Take()
        {
            FD fd = m_fd;
            m_fd.fd = -1;
            return fd;
        }
    };

    // ------------------------------------------------------------------------

    // buffered file stream
    struct Stream { void* ptr; };
    inline bool IsOpen(Stream s) { return s.ptr != 0; }

    // fopen
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/fopen-wfopen
    Stream FOpen(cstr filename, cstr mode, EResult& err);

    // fclose
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/fclose-fcloseall
    void FClose(Stream& stream, EResult& err);

    // fflush
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/fflush
    void FFlush(Stream stream, EResult& err);

    // fread
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/fread
    usize FRead(Stream stream, void* dst, usize size, EResult& err);

    // fwrite
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/fwrite
    usize FWrite(Stream stream, const void* src, usize size, EResult& err);

    // fgets
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/fgets-fgetws
    void FGets(Stream stream, char* dst, usize size, EResult& err);

    // fputs
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/fputs-fputws
    i32 FPuts(Stream stream, cstr src, EResult& err);

    // fileno
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/fileno
    FD FileNo(Stream stream, EResult& err);

    // fdopen
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/fdopen-wfdopen
    Stream FDOpen(FD& hdl, cstr mode, EResult& err);

    // fseek SEEK_SET
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/fseek-lseek-constants
    void FSeek(Stream stream, isize offset, EResult& err);

    // ftell
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/ftell-ftelli64
    i32 FTell(Stream stream, EResult& err);

    // popen
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/popen-wpopen
    Stream POpen(cstr cmd, cstr mode, EResult& err);

    // pclose
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/pclose
    void PClose(Stream& stream, EResult& err);

    // fstat
    inline void FStat(Stream stream, Status& status, EResult& err)
    {
        FD fd = FileNo(stream, err);
        if (err == ESuccess)
        {
            Stat(fd, status, err);
        }
    }

    // fstat.st_size
    inline i64 FSize(Stream stream, EResult& err)
    {
        Status status = {};
        FStat(stream, status, err);
        return status.st_size;
    }

    struct StreamGuard
    {
        Stream m_stream;
        EResult m_result;

        StreamGuard(Stream stream) : m_stream(stream), m_result(EUnknown) {}
        ~StreamGuard() { FClose(m_stream, m_result); }

        inline operator Stream () const
        {
            return m_stream;
        }

        inline Stream Take()
        {
            Stream stream = m_stream;
            m_stream.ptr = 0;
            return stream;
        }
    };

    // ------------------------------------------------------------------------
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/directory-control

    // chdrive
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/chdrive
    void ChDrive(i32 drive, EResult& err);

    // getdrive
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/getdrive
    i32 GetDrive(EResult& err);

    // getdrives
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/getdrives
    u32 GetDrivesMask(EResult& err);

    // getcwd
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/getcwd-wgetcwd
    void GetCwd(char* dst, i32 size, EResult& err);

    // getdcwd
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/getdcwd-wgetdcwd
    void GetDrCwd(i32 drive, char* dst, i32 size, EResult& err);

    // chdir
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/chdir-wchdir
    void ChDir(cstr path, EResult& err);

    // mkdir
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/mkdir-wmkdir
    void MkDir(cstr path, EResult& err);

    // rmdir
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/rmdir-wrmdir
    void RmDir(cstr path, EResult& err);

    enum ChModFlags : u32
    {
        ChMod_Read = 0x0100,
        ChMod_Write = 0x0080,
        ChMod_Exec = 0x0040,
    };
    // chmod
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/chmod-wchmod
    void ChMod(cstr path, u32 flags, EResult& err);

    // ------------------------------------------------------------------------

    // searchenv
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/searchenv-wsearchenv
    void SearchEnv(cstr filename, cstr varname, char(&dst)[260], EResult& err);

    // getenv
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/getenv-wgetenv
    cstr GetEnv(cstr varname, EResult& err);

    // putenv
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/putenv-wputenv
    void PutEnv(cstr varname, cstr value, EResult& err);

    // ------------------------------------------------------------------------

    struct Finder { i32 state; i64 hdl; };
    inline bool IsOpen(Finder fdr) { return fdr.hdl != -1; }

    enum FileAttrFlags : u32
    {
        FAF_Normal = 0x00,
        FAF_ReadOnly = 0x01,
        FAF_Hidden = 0x02,
        FAF_System = 0x04,
        FAF_SubDir = 0x10,
        FAF_Archive = 0x20,
    };

    // __finddata64_t, do not modify
    struct FindData
    {
        u32 attrib;
        i64 time_create;
        i64 time_access;
        i64 time_write;
        i64 size;
        char name[260];
    };

    // findfirst + findnext + findclose
    bool Find(Finder& fdr, FindData& data, cstrc spec, EResult& err);
    void FindAll(Array<FindData>& results, cstrc spec, EResult& err);

    struct Directory
    {
        FindData data;
        Array<Directory> subdirs;
        Array<FindData> files;

        void Reset()
        {
            for (Directory& dir : subdirs)
            {
                dir.Reset();
            }
            subdirs.Reset();
            files.Reset();
        }
    };

    void ListDir(Directory& dir, cstrc spec, EResult& err);

    // ------------------------------------------------------------------------
    struct Module { void* hdl; };

    // https://docs.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-loadlibrarya
    void OpenModule(cstr filename, Module& dst, EResult& err);
    // https://docs.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-freelibrary
    void CloseModule(Module& mod, EResult& err);
    // https://docs.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-getmodulehandlea
    void GetModule(cstr name, Module& dst, EResult& err);
    // https://docs.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-getmodulefilenamea
    void GetModuleName(Module mod, char* dst, u32 dstSize, EResult& err);
    // https://docs.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-getprocaddress
    void GetModuleFunc(Module mod, cstr name, void** pFunc, EResult& err);

    inline void GetExeName(char* dst, u32 dstSize, EResult& err)
    {
        return GetModuleName({ 0 }, dst, dstSize, err);
    }

    inline void GetExeModule(Module& dst, EResult& err)
    {
        return GetModule(0, dst, err);
    }

    // ------------------------------------------------------------------------

    void Curl(cstr url, Array<char>& result, EResult& err);

    // ------------------------------------------------------------------------
}; // IO
