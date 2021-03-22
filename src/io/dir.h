#pragma once

#include "common/macro.h"

PIM_C_BEGIN

bool Dir_Get(char* dst, i32 size);
bool Dir_Set(const char* path);
bool Dir_Add(const char* path);
bool Dir_Rm(const char* path);

typedef enum
{
    ChMod_Read = 0x0100,
    ChMod_Write = 0x0080,
    ChMod_Exec = 0x0040,
} ChModFlags;

bool Dir_ChMod(const char* path, i32 flags);

PIM_C_END
