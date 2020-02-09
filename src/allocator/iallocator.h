#pragma once

#include "common/int_types.h"

struct IAllocator
{
    IAllocator(i32 bytes) {}
    virtual ~IAllocator() {}

    virtual void Clear() = 0;
    virtual void* Alloc(i32 bytes) = 0;
    virtual void Free(void* ptr) = 0;
    virtual void* Realloc(void* prev, i32 bytes) = 0;
};
