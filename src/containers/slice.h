#pragma once

#include "common/macro.h"
#include "common/int_types.h"

template<typename T>
struct Slice
{
    T* ptr;
    i32 len;

    inline bool in_range(i32 i) const { return (u32)i < (u32)len; }
    inline i32 size() const { return len; }
    inline bool empty() const { return len == 0; }
    inline i32 bytes() const { return len * sizeof(T); }

    inline T* begin() { return ptr; }
    inline T* end() { return ptr + len; }
    inline const T* begin() const { return ptr; }
    inline const T* end() const { return ptr + len; }
    inline T& front() { DebugAssert(len > 0); return ptr[0]; }
    inline const T& front() const { DebugAssert(len > 0); return ptr[0]; }
    inline T& back() { DebugAssert(len > 0); return ptr[len - 1]; }
    inline const T& back() const { DebugAssert(len > 0); return ptr[len - 1]; }
    inline T& operator[](i32 i) { DebugAssert(in_range(i)); return ptr[i]; }
    inline const T& operator[](i32 i) const { DebugAssert(in_range(i)); return ptr[i]; }

    template<typename U>
    inline Slice<U> cast() const
    {
        return { (U*)ptr, bytes() / (i32)sizeof(U) };
    }
    template<typename U>
    inline explicit operator Slice<U>() const
    {
        return { (U*)ptr, bytes() / (i32)sizeof(U) };
    }
    inline operator Slice<const T>() const
    {
        return { (const T*)ptr, len };
    }
    inline Slice<T> subslice(i32 start, i32 count) const
    {
        DebugAssert(in_range(start + count));
        return { ptr + start, count };
    }
    inline Slice<T> head(i32 last) const
    {
        DebugAssert(last >= 0);
        DebugAssert(last <= len);
        return { ptr, last };
    }
    inline Slice<T> tail(i32 first) const
    {
        DebugAssert(first >= 0);
        DebugAssert(first <= len);
        return { ptr + first, len - first };
    }
};
