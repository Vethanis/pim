#pragma once

#include "common/macro.h"

PIM_C_BEGIN

i32 dir_errno(void);

void pim_getcwd(char* dst, i32 size);
void pim_chdir(const char* path);
void pim_mkdir(const char* path);
void pim_rmdir(const char* path);

typedef enum
{
    ChMod_Read = 0x0100,
    ChMod_Write = 0x0080,
    ChMod_Exec = 0x0040,
} ChModFlags;

void pim_chmod(const char* path, i32 flags);

PIM_C_END
