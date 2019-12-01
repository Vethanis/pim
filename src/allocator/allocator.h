#pragma once

#include "common/macro.h"
#include "common/int_types.h"
#include "containers/slice.h"

struct Allocation
{
    Slice<u8> slice;
    AllocType type;

    inline u8* begin() { return slice.begin(); }
    inline u8* end() { return slice.end(); }
    inline const u8* begin() const { return slice.begin(); }
    inline const u8* end() const { return slice.end(); }
    inline i32 Size() const { return slice.Size(); }
};

template<typename T>
struct AllocationT
{
    Slice<T> slice;
    AllocType type;

    inline T* begin() { return slice.begin(); }
    inline T* end() { return slice.end(); }
    inline const T* begin() const { return slice.begin(); }
    inline const T* end() const { return slice.end(); }
    inline i32 Size() const { return slice.Size(); }
};

namespace Allocator
{
    void Init();
    void Update();
    void Shutdown();

    void* ImGuiAllocFn(size_t sz, void*);
    void  ImGuiFreeFn(void* ptr, void*);

    template<typename T>
    inline Allocation ToUntyped(AllocationT<T> allocT)
    {
        return
        {
            allocT.slice.Cast<u8>(),
            allocT.type,
        };
    }

    template<typename T>
    inline AllocationT<T> ToTyped(Allocation alloc)
    {
        return
        {
            alloc.slice.Cast<T>(),
            alloc.type,
        };
    }

    Allocation Alloc(AllocType type, i32 bytes);
    void Realloc(Allocation& alloc, i32 bytes);
    void Free(Allocation& prev);

    template<typename T>
    inline AllocationT<T> AllocT(AllocType type, i32 count)
    {
        Assert(count >= 0);
        Allocation alloc = Alloc(type, (i32)sizeof(T) * count);
        return ToTyped<T>(alloc);
    }

    template<typename T>
    inline void ReallocT(AllocationT<T>& allocT, i32 count)
    {
        Assert(count >= 0);
        Allocation alloc = ToUntyped(allocT);
        Realloc(alloc, (i32)sizeof(T) * count);
        allocT = ToTyped<T>(alloc);
    }

    template<typename T>
    inline void FreeT(AllocationT<T>& allocT)
    {
        Allocation alloc = ToUntyped(allocT);
        Free(alloc);
        allocT = ToTyped<T>(alloc);
    }

    inline void Reserve(Allocation& alloc, i32 count)
    {
        Assert(count >= 0);
        const i32 cur = alloc.Size();
        if (count > cur)
        {
            i32 next = cur << 1;
            next = Max(next, 64);
            next = Max(next, count);
            Realloc(alloc, next);
        }
    }

    template<typename T>
    inline void ReserveT(AllocationT<T>& alloc, i32 count)
    {
        Assert(count >= 0);
        const i32 cur = alloc.Size();
        if (count > cur)
        {
            i32 next = cur << 1;
            next = Max(next, 8);
            next = Max(next, count);
            ReallocT(alloc, next);
        }
    }
};
