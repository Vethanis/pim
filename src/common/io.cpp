
#define _CRT_SECURE_NO_WARNINGS

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include <stdio.h>
#include <errno.h>

#include "common/io.h"
#include "common/macro.h"

namespace IO
{
    FD Create(cstr filename)
    {
        i32 fd = _creat(filename, _S_IREAD | _S_IWRITE);
        return { fd };
    }
    FD Open(cstr filename, OFlagBits flags)
    {
        i32 fd = _open(filename, (i32)flags.bits, _S_IREAD | _S_IWRITE);
        return { fd };
    }
    bool Close(FD& hdl)
    {
        i32 rval = -1;
        if (hdl.fd > 2)
        {
            rval = _close(hdl.fd);
            DebugAssert(!rval);
        }
        hdl.fd = -1;
        return !rval;
    }
    i32 Read(FD hdl, void* dst, i32 size)
    {
        DebugAssert(IsValid(hdl));
        DebugAssert(dst);
        i32 rval = _read(hdl.fd, dst, size);
        DebugAssert(rval >= 0);
        return rval;
    }
    i32 Write(FD hdl, const void* src, i32 size)
    {
        DebugAssert(IsValid(hdl));
        DebugAssert(src);
        i32 rval = _write(hdl.fd, src, size);
        DebugAssert(rval >= 0);
        return rval;
    }
    bool Pipe(FD& p0, FD& p1, i32 bufferSize)
    {
        i32 fds[2] = { -1, -1 };
        i32 rval = _pipe(fds, bufferSize, _O_BINARY);
        DebugAssert(!rval);
        p0.fd = fds[0];
        p1.fd = fds[1];
        return !rval;
    }
    bool Stat(FD hdl, Status& status)
    {
        status = {};
        i32 rval = _fstat64(hdl.fd, (struct _stat64*)&status);
        DebugAssert(!rval);
        return !rval;
    }

    // ------------------------------------------------------------------------

    Writer::State Writer::Start(FD hdl, const void* src, i32 size)
    {
        DebugAssert(IsValid(hdl));
        DebugAssert(src);
        DebugAssert(size > 0);
        DebugAssert(m_state == State::Idle);

        m_hdl = hdl;
        m_src = (const u8*)src;
        m_size = size;
        m_state = IsValid(hdl) ? State::Writing : State::Error;

        return m_state;
    }
    Writer::State Writer::Update()
    {
        DebugAssert(m_state == State::Writing);

        i32 wrote = Write(m_hdl, m_src, Min(WriteSize, m_size));
        if (wrote < 0)
        {
            m_state = State::Error;
        }
        else
        {
            m_src += wrote;
            m_size -= wrote;
            if (m_size <= 0)
            {
                Close(m_hdl);
                m_state = State::Idle;
            }
        }
        return m_state;
    }

    // ------------------------------------------------------------------------

    Reader::State Reader::Start(FD hdl)
    {
        DebugAssert(IsValid(hdl));
        DebugAssert(m_state == State::Idle);

        m_hdl = hdl;
        m_state = IsValid(hdl) ? State::Reading : State::Error;

        return m_state;
    }
    Reader::State Reader::Update(Slice<u8>& result)
    {
        DebugAssert(m_state == State::Reading);

        result = { 0, 0 };
        i32 bytesRead = Read(m_hdl, m_dst, ReadSize);
        if (bytesRead > 0)
        {
            result = { m_dst, bytesRead };
        }
        else if (bytesRead == 0)
        {
            Close(m_hdl);
            m_state = State::Idle;
        }
        else
        {
            m_state = State::Error;
        }
        return m_state;
    }

    // ------------------------------------------------------------------------

    Stream FOpen(cstr filename, cstr mode)
    {
        DebugAssert(filename);
        DebugAssert(mode);
        FILE* ptr = fopen(filename, mode);
        return { ptr };
    }
    bool FClose(Stream& stream)
    {
        i32 rval = -1;
        if (IsOpen(stream))
        {
            rval = fclose((FILE*)stream.ptr);
            DebugAssert(!rval);
        }
        stream.ptr = 0;
        return !rval;
    }
    bool FFlush(Stream stream)
    {
        DebugAssert(IsOpen(stream));
        i32 rval = fflush((FILE*)stream.ptr);
        DebugAssert(!rval);
        return !rval;
    }
    usize FRead(Stream stream, void* dst, i32 size)
    {
        DebugAssert(IsOpen(stream));
        DebugAssert(dst);
        usize rval = fread(dst, size, 1, (FILE*)stream.ptr);
        DebugEAssert();
        return rval;
    }
    usize FWrite(Stream stream, const void* src, i32 size)
    {
        DebugAssert(IsOpen(stream));
        DebugAssert(src);
        usize rval = fwrite(src, size, 1, (FILE*)stream.ptr);
        DebugEAssert();
        return rval;
    }
    char* FGets(Stream stream, char* dst, i32 size)
    {
        DebugAssert(IsOpen(stream));
        DebugAssert(dst);
        char* rval = fgets(dst, size, (FILE*)stream.ptr);
        DebugEAssert();
        return rval;
    }
    i32 FPuts(Stream stream, cstr src)
    {
        i32 rval = fputs(src, (FILE*)stream.ptr);
        DebugEAssert();
        return rval;
    }
    FD FileNo(Stream stream)
    {
        DebugAssert(IsOpen(stream));
        i32 fd = _fileno((FILE*)stream.ptr);
        FD hdl = { fd };
        DebugAssert(IsValid(hdl));
        return hdl;
    }
    Stream FDOpen(FD& hdl, cstr mode)
    {
        DebugAssert(IsValid(hdl));
        FILE* ptr = _fdopen(hdl.fd, mode);
        DebugAssert(ptr);
        if (ptr)
        {
            hdl.fd = -1;
        }
        return { ptr };
    }
    bool FSeek(Stream stream, i32 offset)
    {
        DebugAssert(IsOpen(stream));
        i32 rval = fseek((FILE*)stream.ptr, offset, SEEK_SET);
        DebugAssert(!rval);
        return !rval;
    }
    i32 FTell(Stream stream)
    {
        DebugAssert(IsOpen(stream));
        i32 rval = ftell((FILE*)stream.ptr);
        DebugAssert(rval != -1);
        return rval;
    }
    Stream POpen(cstr cmd, cstr mode)
    {
        DebugAssert(cmd);
        DebugAssert(mode);
        FILE* ptr = _popen(cmd, mode);
        DebugAssert(ptr);
        return { ptr };
    }
    bool PClose(Stream& stream)
    {
        i32 rval = -1;
        if (IsOpen(stream))
        {
            rval = _pclose((FILE*)stream.ptr);
            DebugAssert(!rval);
        }
        stream.ptr = 0;
        return !rval;
    }
}; // IO
