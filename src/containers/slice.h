#pragma once

#include "common/macro.h"
#include "common/int_types.h"

template<typename T>
struct Slice
{
    T* ptr;
    i32 len;

    inline bool InRange(i32 i) const { return (u32)i < (u32)len; }
    inline i32 Size() const { return len; }
    inline bool IsEmpty() const { return len == 0; }
    inline i32 Bytes() const { return len * sizeof(T); }

    inline T* begin() { return ptr; }
    inline T* end() { return ptr + len; }
    inline const T* begin() const { return ptr; }
    inline const T* end() const { return ptr + len; }
    inline T& Front() { ASSERT(len > 0); return ptr[0]; }
    inline const T& Front() const { ASSERT(len > 0); return ptr[0]; }
    inline T& Back() { ASSERT(len > 0); return ptr[len - 1]; }
    inline const T& Back() const { ASSERT(len > 0); return ptr[len - 1]; }
    inline T& operator[](i32 i) { ASSERT(InRange(i)); return ptr[i]; }
    inline const T& operator[](i32 i) const { ASSERT(InRange(i)); return ptr[i]; }

    template<typename U>
    inline Slice<U> Cast() const
    {
        return
        {
            (U*)begin(),
            Bytes() / (i32)sizeof(U)
        };
    }

    inline Slice<u8> ToBytes() const
    {
        return { (u8*)ptr, (i32)sizeof(T) * len };
    }

    inline operator Slice<const T>() const
    {
        return { ptr, len };
    }

    inline Slice<T> Subslice(i32 start, i32 count) const
    {
        ASSERT(InRange(start + count));
        return { ptr + start, count };
    }
    inline Slice<T> Head(i32 last) const
    {
        ASSERT(last >= 0);
        ASSERT(last <= len);
        return { ptr, last };
    }
    inline Slice<T> Tail(i32 first) const
    {
        ASSERT(first >= 0);
        ASSERT(first <= len);
        return { ptr + first, len - first };
    }

    inline bool Overlaps(const Slice<T> other) const
    {
        return (end() >= other.begin()) && (begin() <= other.end());
    }
};
