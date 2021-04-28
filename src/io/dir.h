#pragma once

#include "common/macro.h"

typedef enum
{
    ChMod_Read = 0x0100,
    ChMod_Write = 0x0080,
    ChMod_Exec = 0x0040,
} ChModFlags;

PIM_C_BEGIN

bool IO_GetCwd(char* dst, i32 size);
bool IO_ChDir(const char* path);
bool IO_MkDir(const char* path);
bool IO_RmDir(const char* path);
bool IO_ChMod(const char* path, i32 flags);

PIM_C_END
