#pragma once

#include "common/macro.h"
#include "common/int_types.h"
#include "containers/slice.h"
#include "containers/array.h"

namespace IO
{
    i32 GetErrNo();
    void SetErrNo(i32 value);
    void ClearErrNo();

    // offset within a file and number of bytes
    struct Chunk
    {
        i32 offset;
        i32 length;
    };

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
    FD Create(cstr filename);

    // open
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/open-wopen
    FD Open(cstr filename, u32 flags);

    // close
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/close
    bool Close(FD& hdl);

    // read
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/read
    i32 Read(FD hdl, void* dst, usize size);

    // write
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/write
    i32 Write(FD hdl, const void* src, usize size);

    // seek
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/lseek-lseeki64
    i32 Seek(FD hdl, isize offset);

    // tell
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/tell-telli64
    i32 Tell(FD hdl);

    // pipe
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/pipe
    bool Pipe(FD& p0, FD& p1, usize bufferSize);

    // fstat
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/fstat-fstat32-fstat64-fstati64-fstat32i64-fstat64i32
    bool Stat(FD hdl, Status& status);

    // fstat.st_size
    inline i64 Size(FD fd)
    {
        Status status = {};
        Stat(fd, status);
        return status.st_size;
    }

    struct FDGuard
    {
        FD& m_fd;
        FDGuard(FD& fd) : m_fd(fd) {}
        ~FDGuard() {  Close(m_fd); }
        inline void Cancel() { m_fd.fd = -1; }
    };

    // ------------------------------------------------------------------------

    // buffered file stream
    struct Stream { void* ptr; };
    inline bool IsOpen(Stream s) { return s.ptr != 0; }

    // fopen
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/fopen-wfopen
    Stream FOpen(cstr filename, cstr mode);

    // fclose
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/fclose-fcloseall
    bool FClose(Stream& stream);

    // fflush
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/fflush
    bool FFlush(Stream stream);

    // fread
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/fread
    usize FRead(Stream stream, void* dst, usize size);

    // fwrite
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/fwrite
    usize FWrite(Stream stream, const void* src, usize size);

    // fgets
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/fgets-fgetws
    char* FGets(Stream stream, char* dst, usize size);

    // fputs
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/fputs-fputws
    i32 FPuts(Stream stream, cstr src);

    // fileno
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/fileno
    FD FileNo(Stream stream);

    // fdopen
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/fdopen-wfdopen
    Stream FDOpen(FD& hdl, cstr mode);

    // fseek SEEK_SET
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/fseek-lseek-constants
    bool FSeek(Stream stream, isize offset);

    // ftell
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/ftell-ftelli64
    i32 FTell(Stream stream);

    // popen
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/popen-wpopen
    Stream POpen(cstr cmd, cstr mode);

    // pclose
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/pclose
    bool PClose(Stream& stream);

    // fstat
    inline bool FStat(Stream stream, Status& status)
    {
        return Stat(FileNo(stream), status);
    }

    // fstat.st_size
    inline i64 FSize(Stream stream)
    {
        Status status = {};
        FStat(stream, status);
        return status.st_size;
    }

    struct StreamGuard
    {
        Stream& m_stream;
        StreamGuard(Stream& stream) : m_stream(stream) {}
        ~StreamGuard() { FClose(m_stream); }
        inline bool Cancel() { m_stream.ptr = 0; }
    };

    // ------------------------------------------------------------------------
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/directory-control

    // chdrive
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/chdrive
    bool ChDrive(i32 drive);

    // getdrive
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/getdrive
    i32 GetDrive();

    // getdrives
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/getdrives
    u32 GetDrivesMask();

    // getcwd
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/getcwd-wgetcwd
    bool GetCwd(char* dst, i32 size);

    // getdcwd
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/getdcwd-wgetdcwd
    bool GetDrCwd(i32 drive, char* dst, i32 size);

    // chdir
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/chdir-wchdir
    bool ChDir(cstr path);

    // mkdir
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/mkdir-wmkdir
    bool MkDir(cstr path);

    // rmdir
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/rmdir-wrmdir
    bool RmDir(cstr path);

    enum ChModFlags : u32
    {
        ChMod_Read = 0x0100,
        ChMod_Write = 0x0080,
        ChMod_Exec = 0x0040,
    };
    // chmod
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/chmod-wchmod
    bool ChMod(cstr path, u32 flags);

    // ------------------------------------------------------------------------

    // searchenv
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/searchenv-wsearchenv
    bool SearchEnv(cstr filename, cstr varname, char(&dst)[260]);

    // getenv
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/getenv-wgetenv
    cstr GetEnv(cstr varname);

    // putenv
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/putenv-wputenv
    bool PutEnv(cstr varname, cstr value);

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

    // __finddata64_t
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
    bool Find(Finder& fdr, FindData& data, cstrc spec);
    void FindAll(Array<FindData>& results, cstrc spec);

    // ------------------------------------------------------------------------
    struct Module { void* hdl; };

    // https://docs.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-loadlibrarya
    bool OpenModule(cstr filename, Module& dst);
    // https://docs.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-freelibrary
    bool CloseModule(Module& mod);
    // https://docs.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-getmodulehandlea
    bool GetModule(cstr name, Module& dst);
    // https://docs.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-getmodulefilenamea
    bool GetModuleName(Module mod, char* dst, u32 dstSize);
    // https://docs.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-getprocaddress
    void* GetModuleFunc(Module mod, cstr name);

    inline bool GetExeName(char* dst, u32 dstSize)
    {
        return GetModuleName({ 0 }, dst, dstSize);
    }

    inline bool GetExeModule(Module& dst)
    {
        return GetModule(0, dst);
    }

    // ------------------------------------------------------------------------
}; // IO
