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
    inline T& Front() { Assert(len > 0); return ptr[0]; }
    inline const T& Front() const { Assert(len > 0); return ptr[0]; }
    inline T& Back() { Assert(len > 0); return ptr[len - 1]; }
    inline const T& Back() const { Assert(len > 0); return ptr[len - 1]; }
    inline T& operator[](i32 i) { Assert(InRange(i)); return ptr[i]; }
    inline const T& operator[](i32 i) const { Assert(InRange(i)); return ptr[i]; }

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
        return Cast<u8>();
    }

    inline operator Slice<const T>() const
    {
        return { ptr, len };
    }

    inline Slice<T> Subslice(i32 start, i32 count) const
    {
        Assert(InRange(start + count));
        return { ptr + start, count };
    }
    inline Slice<T> Head(i32 last) const
    {
        Assert(last >= 0);
        Assert(last <= len);
        return { ptr, last };
    }
    inline Slice<T> Tail(i32 first) const
    {
        Assert(first >= 0);
        Assert(first <= len);
        return { ptr + first, len - first };
    }

    inline i32 Find(T key) const
    {
        T* p = ptr;
        i32 l = len;
        for (i32 i = 0; i < l; ++i)
        {
            if (p[i] == key)
            {
                return i;
            }
        }
        return -1;
    }

    inline i32 RFind(T key) const
    {
        T* p = ptr;
        i32 l = len;
        for (i32 i = l - 1; i >= 0; --i)
        {
            if (p[i] == key)
            {
                return i;
            }
        }
        return -1;
    }
};
