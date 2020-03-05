#pragma once

#include "common/macro.h"
#include "common/int_types.h"

template<typename T>
struct Slice
{
    T* ptr;
    i32 len;

    inline bool InRange(i32 i) const { return (u32)i < (u32)len; }
    inline bool IsEmpty() const { return len == 0; }

    inline i32 bytes() const { return len * sizeof(T); }
    inline i32 size() const { return len; }
    inline T* begin() { return ptr; }
    inline T* end() { return ptr + len; }
    inline const T* begin() const { return ptr; }
    inline const T* end() const { return ptr + len; }
    inline T& front() { ASSERT(len > 0); return ptr[0]; }
    inline const T& front() const { ASSERT(len > 0); return ptr[0]; }
    inline T& back() { ASSERT(len > 0); return ptr[len - 1]; }
    inline const T& back() const { ASSERT(len > 0); return ptr[len - 1]; }
    inline T& operator[](i32 i) { ASSERT(InRange(i)); return ptr[i]; }
    inline const T& operator[](i32 i) const { ASSERT(InRange(i)); return ptr[i]; }

    template<typename U>
    inline Slice<U> Cast() const
    {
        return
        {
            (U*)begin(),
            bytes() / (i32)sizeof(U)
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
};

template<typename T>
static bool Overlaps(Slice<T> lhs, Slice<T> rhs)
{
    return (lhs.begin() <= rhs.end()) && (lhs.end() >= rhs.begin());
}

template<typename T>
static bool Adjacent(Slice<T> lhs, Slice<T> rhs)
{
    return (lhs.begin() == rhs.end()) || (rhs.begin() == lhs.end());
}

template<typename T>
static Slice<T> Combine(Slice<T> lhs, Slice<T> rhs)
{
    ASSERT(Adjacent(lhs, rhs));
    return
    {
        lhs.ptr < rhs.ptr ? lhs.ptr : rhs.ptr,
        lhs.len + rhs.len
    };
}

template<typename T>
static void Copy(Slice<T> dst, Slice<T> src)
{
    const i32 sz = a < b ? a : b;
    for (i32 i = 0; i < sz; ++i)
    {
        dst[i] = src[i];
    }
}
