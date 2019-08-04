#pragma once

#include "common/int_types.h"
#include "common/macro.h"

template<typename T>
struct Slice
{
    T* ptr;
    i32 len;

    inline bool empty() { return len == 0; }
    inline i32 size() { return len; }
    inline usize bytes() { return (usize)len * sizeof(T); }
    inline T* begin() { return ptr; }
    inline T* end() { return ptr + len; }
    inline const T* begin() const { return ptr; }
    inline const T* end() const { return ptr + len; }
    inline T& front() { DebugAssert(!empty()); return ptr[0]; }
    inline const T& front() const { DebugAssert(!empty()); return ptr[0]; }
    inline T& back() { DebugAssert(!empty()); return ptr[len - 1]; }
    inline const T& back() const { DebugAssert(!empty()); return ptr[len - 1]; }
    inline T& operator[](i32 i) { DebugAssert((u32)i < (u32)len); return ptr[i]; }
    inline const T& operator[](i32 i) const { DebugAssert((u32)i < (u32)len); return ptr[i]; }
    
    template<typename U>
    inline Slice<U> cast() { return { (U*)ptr, bytes() / sizeof(U) }; }
    inline Slice<T> subslice(i32 start, i32 count)
    {
        DebugAssert((start + count) < len);
        return { ptr + start, ptr + start + count };
    }
};
