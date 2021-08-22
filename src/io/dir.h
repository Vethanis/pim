#pragma once

#include "common/macro.h"

PIM_C_BEGIN

bool IO_GetCwd(char* dst, i32 size);
bool IO_ChDir(const char* path);
bool IO_MkDir(const char* path);
bool IO_RmDir(const char* path);
bool IO_ChMod(const char* path, i32 flags);

PIM_C_END
