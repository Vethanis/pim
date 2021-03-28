#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "io/fd.h"

typedef struct FStream_s
{
    void* handle;
} FStream;

bool FStream_IsOpen(FStream fstr);
FStream FStream_Open(const char* filename, const char* mode);
bool FStream_Close(FStream* stream);
bool FStream_Flush(FStream stream);
i32 FStream_Read(FStream stream, void* dst, i32 size);
i32 FStream_Write(FStream stream, const void* src, i32 size);
bool FStream_Gets(FStream stream, char* dst, i32 size);
i32 FStream_Puts(FStream stream, const char* src);
i32 FStream_VPrintf(FStream stream, const char* fmt, va_list ap);
i32 FStream_Printf(FStream stream, const char* fmt, ...);

fd_t FStream_ToFd(FStream stream);
FStream Fd_ToFStream(fd_t* hdl, const char* mode);

bool FStream_Seek(FStream stream, i32 offset);
i32 FStream_Tell(FStream stream);

FStream FStream_POpen(const char* cmd, const char* mode);
bool FStream_PClose(FStream* stream);

bool FStream_Stat(FStream stream, fd_status_t* status);
i64 FStream_Size(FStream stream);

PIM_C_END
