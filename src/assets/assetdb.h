#pragma once

#include "common/int_types.h"

struct Asset
{
    cstr name;
    cstr pack;
    i32 offset;
    i32 size;
    void* pData;
    i32 refCount;
};
