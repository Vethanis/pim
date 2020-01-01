#pragma once

#include "common/macro.h"
#include "common/int_types.h"
#include "common/minmax.h"

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

    inline bool ValidSlice(i32 start, i32 count) const
    {
        const i32 endIndex = start + count;
        ASSERT(start >= 0);
        ASSERT(count >= 0);
        ASSERT(endIndex >= 0);
        return (u32)endIndex <= (u32)len;
    }
    inline Slice<T> Subslice(i32 start, i32 count) const
    {
        ASSERT(ValidSlice(start, count));
        ASSERT(ptr || !(start | count));
        return { ptr + start, count };
    }
    inline Slice<T> Head(i32 last) const
    {
        return Subslice(0, last);
    }
    inline Slice<T> Tail(i32 first) const
    {
        return Subslice(first, len - first);
    }

    inline bool Overlaps(const Slice<T> other) const
    {
        return ::Overlaps(begin(), end(), other.begin(), other.end());
    }

    inline bool Adjacent(const Slice<T> other) const
    {
        return ::Adjacent(begin(), end(), other.begin(), other.end());
    }

    inline void Combine(Slice<T> other)
    {
        ASSERT(Adjacent(other));
        ptr = ptr < other.ptr ? ptr : other.ptr;
        len = len + other.len;
        ASSERT(len >= 0);
    }
};
