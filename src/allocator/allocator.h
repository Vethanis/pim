#pragma once

#include "common/macro.h"
#include "common/minmax.h"
#include "common/int_types.h"
#include <string.h>

namespace Allocator
{
    void Init();
    void Update();
    void Shutdown();

    void* Alloc(AllocType type, i32 bytes);
    void Free(void* prev);
    void* Realloc(AllocType type, void* prev, i32 bytes);

    inline void* Calloc(AllocType type, i32 bytes)
    {
        void* ptr = Alloc(type, bytes);
        memset(ptr, 0, bytes);
        return ptr;
    }

    template<typename T>
    inline T* AllocT(AllocType type, i32 count)
    {
        return (T*)Alloc(type, sizeof(T) * count);
    }

    template<typename T>
    inline T* ReallocT(AllocType type, T* prev, i32 count)
    {
        return (T*)Realloc(type, prev, sizeof(T) * count);
    }

    template<typename T>
    inline T* CallocT(AllocType type, i32 count)
    {
        return (T*)Calloc(type, sizeof(T) * count);
    }

    template<typename T>
    inline T* Reserve(AllocType allocator, T* pOld, i32& capacity, i32 want)
    {
        const i32 oldCapacity = capacity;
        ASSERT(want >= 0);
        ASSERT(oldCapacity >= 0);

        if (want > oldCapacity)
        {
            const i32 newCapacity = Max(want, Max(oldCapacity * 2, 16));
            capacity = newCapacity;
            return ReallocT<T>(allocator, pOld, newCapacity);
        }

        return pOld;
    }

    inline void* ImGuiAllocFn(size_t sz, void*) { return Alloc(Alloc_Pool, (i32)sz); }
    inline void ImGuiFreeFn(void* ptr, void*) { Free(ptr); }
};
