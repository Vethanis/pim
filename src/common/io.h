#pragma once

#include "common/int_types.h"
#include "containers/slice.h"

namespace IO
{
    // file descriptor
    // https://en.wikipedia.org/wiki/File_descriptor
    struct FD { i32 fd; };

    static constexpr FD Invalid = { -1 };
    static constexpr FD StdIn = { 0 };
    static constexpr FD StdOut = { 1 };
    static constexpr FD StdErr = { 2 };

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
    struct OFlagBits { u32 bits; };

    inline bool IsValid(FD hdl) { return hdl.fd >= 0; }
    inline bool IsInvalid(FD hdl) { return hdl.fd < 0; }

    // creat
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/creat-wcreat
    FD Create(cstr filename);
    // open
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/open-wopen
    FD Open(cstr filename, OFlagBits flags);
    // close
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/close
    bool Close(FD& hdl);
    // read
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/read
    i32 Read(FD hdl, void* dst, i32 size);
    // write
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/write
    i32 Write(FD hdl, const void* src, i32 size);
    // pipe
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/pipe
    bool Pipe(FD& p0, FD& p1, i32 bufferSize);
    // fstat
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/fstat-fstat32-fstat64-fstati64-fstat32i64-fstat64i32
    bool Stat(FD hdl, Status& status);

    // ------------------------------------------------------------------------

    struct Writer
    {
        static constexpr i32 WriteSize = 1024 * 4;

        enum State : i32
        {
            Idle = 0,
            Writing,
            Error,
        };

        const u8* m_src;
        i32 m_size;
        FD m_hdl;
        State m_state;

        State Start(FD hdl, const void* src, i32 size);
        State Update();
    }; // Writer

    struct Reader
    {
        static constexpr i32 ReadSize = 1024 * 4;

        enum State : i32
        {
            Idle = 0,
            Reading,
            Error,
        };

        u8 m_dst[ReadSize];
        FD m_hdl;
        State m_state;

        State Start(FD hdl);
        State Update(Slice<u8>& result);
    }; // Reader

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
    usize FRead(Stream stream, void* dst, i32 size);
    // fwrite
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/fwrite
    usize FWrite(Stream stream, const void* src, i32 size);
    // fgets
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/fgets-fgetws
    char* FGets(Stream stream, char* dst, i32 size);
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
    bool FSeek(Stream stream, i32 offset);
    // ftell
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/ftell-ftelli64
    i32 FTell(Stream stream);
    // popen
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/popen-wpopen
    Stream POpen(cstr cmd, cstr mode);
    // pclose
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/pclose
    bool PClose(Stream& stream);
}; // IO
