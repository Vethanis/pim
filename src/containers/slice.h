#pragma once

#include "common/macro.h"
#include "common/int_types.h"

template<typename T>
struct Slice
{
    T* ptr;
    usize len;

    inline usize size() { return len; }
    inline bool empty() { return size() == 0; }
    inline usize bytes() { return size() * sizeof(T); }
    inline T* begin() { return ptr; }
    inline T* end() { return begin() + size(); }
    inline const T* begin() const { return ptr; }
    inline const T* end() const { return begin() + size(); }
    inline T& front() { DebugAssert(!empty()); return begin()[0]; }
    inline const T& front() const { DebugAssert(!empty()); return begin()[0]; }
    inline T& back() { DebugAssert(!empty()); return begin()[size() - 1]; }
    inline const T& back() const { DebugAssert(!empty()); return begin()[size() - 1]; }
    inline T& operator[](usize i) { DebugAssert(i < size()); return begin()[i]; }
    inline const T& operator[](usize i) const { DebugAssert(i < size()); return begin()[i]; }

    template<typename U>
    inline Slice<U> cast()
    {
        return { (U*)begin(), bytes() / sizeof(U) };
    }
    inline Slice<T> subslice(usize start, usize count)
    {
        DebugAssert((start + count) < size());
        return { begin() + start, count };
    }
};
