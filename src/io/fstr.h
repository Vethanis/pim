#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "io/fd.h"

typedef struct FileStream_s
{
    void* handle;
} FileStream;

bool FileStream_IsOpen(FileStream fstr);
FileStream FileStream_Open(const char* filename, const char* mode);
bool FileStream_Close(FileStream* stream);
bool FileStream_Flush(FileStream stream);
i32 FileStream_Read(FileStream stream, void* dst, i32 size);
i32 FileStream_Write(FileStream stream, const void* src, i32 size);
bool FileStream_Gets(FileStream stream, char* dst, i32 size);
i32 FileStream_Puts(FileStream stream, const char* src);

fd_t FileStream_ToFd(FileStream stream);
FileStream Fd_ToFileStream(fd_t* hdl, const char* mode);

bool FileStream_Seek(FileStream stream, i32 offset);
i32 FileStream_Tell(FileStream stream);

FileStream FileStream_ProcOpen(const char* cmd, const char* mode);
bool FileStream_ProcClose(FileStream* stream);

bool FileStream_Stat(FileStream stream, fd_status_t* status);
i64 FileStream_Size(FileStream stream);

PIM_C_END
