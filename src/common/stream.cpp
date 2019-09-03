#include "common/stream.h"
#include "common/macro.h"

#define _CRT_SECURE_NO_WARNINGS 1

#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/stat.h>

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

#define GetFile()   (FILE*)m_handle
#define Check(x)    DebugAssert(m_handle); x; DebugEAssert();
#define CheckRet(x) DebugAssert(m_handle); auto y = x; DebugEAssert(); return y;

Stream Stream::Open(cstr path, u32 mode)
{
    char modeStr[16];
    ModeToStr(modeStr, mode);
    FILE* file = 0;
    fopen_s(&file, path, modeStr);
    DebugEAssert();
    return { file };
}

void Stream::Close()
{
    Check(fclose(GetFile()));
}

void Stream::Flush()
{
    Check(fflush(GetFile()));
}

i32 Stream::GetFileNumber() const
{
    CheckRet(_fileno(GetFile()));
}

i32 Stream::Size() const
{
    DebugAssert(m_handle);
    struct stat s = {};
    fstat(GetFileNumber(), &s);
    DebugEAssert();
    return s.st_size;
}

i32 Stream::Tell() const
{
    CheckRet(ftell(GetFile()));
}

void Stream::Rewind()
{
    Check(rewind(GetFile()));
}

void Stream::Seek(i32 offset)
{
    Check(fseek(GetFile(), offset, SEEK_SET));
}

bool Stream::AtEnd() const
{
    CheckRet(feof(GetFile()));
}

i32 Stream::GetChar()
{
    CheckRet(fgetc(GetFile()));
}

void Stream::PutChar(i32 c)
{
    Check(fputc(c, GetFile()));
}

void Stream::PutStr(cstr src)
{
    DebugAssert(src);
    Check(fputs(src, GetFile()));
}

void Stream::Print(cstr fmt, ...)
{
    DebugAssert(m_handle);
    DebugAssert(fmt);

    va_list lst;
    va_start(lst, fmt);
    vfprintf(GetFile(), fmt, lst);
    DebugEAssert();

    va_end(lst);
}

void Stream::Scan(cstr fmt, ...)
{
    DebugAssert(m_handle);
    DebugAssert(fmt);

    va_list lst;
    va_start(lst, fmt);
    vfscanf((FILE*)m_handle, fmt, lst);
    DebugEAssert();

    va_end(lst);
}

void Stream::Write(const void* src, usize len)
{
    Check(fwrite(src, len, 1, GetFile()));
}

void Stream::Read(void* dst, usize len)
{
    Check(fread(dst, len, 1, GetFile()));
}
