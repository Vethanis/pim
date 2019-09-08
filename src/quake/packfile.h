#pragma once

#include "common/int_types.h"

// common.c 1619
// COM_LoadPackFile
// pack_t

namespace Quake
{
    struct PackFile
    {
        char name[56];
        i32 filePos;
        i32 fileLen;
    };

    struct PackHeader
    {
        char id[4];
        i32 dirOffset;
        i32 dirLen;
    };
};
