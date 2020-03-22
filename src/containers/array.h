#pragma once

#include "common/macro.h"
#include "common/int_types.h"
#include "common/minmax.h"
#include "allocator/allocator.h"
#include "containers/slice.h"
#include <string.h>

template<typename T>
struct Array
{
private:
    T* m_ptr;
    i32 m_length;
    u32 m_bits;

    static constexpr u32 kCapMask = 0x0fffffff;
    static constexpr u32 kAllocMask = ~kCapMask;
    static constexpr u32 kAllocShift = 30;

    SASSERT(EAlloc_Count <= 0xf);

    i32 GetCapacity() const
    {
        u32 x = m_bits & kCapMask;
        return (i32)x;
    }

    void SetCapacity(i32 c)
    {
        ASSERT((u32)c <= kCapMask);
        m_bits = (kAllocMask & m_bits) | (kCapMask & c);
    }

    void SetAllocator(EAlloc allocator)
    {
        ASSERT(allocator <= EAlloc_Count);
        u32 a = 0xf & allocator;
        m_bits = (a << kAllocShift) | (kCapMask & m_bits);
    }

public:

    EAlloc GetAllocator() const
    {
        u32 x = ((m_bits & kAllocMask) >> kAllocShift);
        ASSERT(x < EAlloc_Count);
        return (EAlloc)x;
    }

    i32 capacity() const { return GetCapacity(); }
    i32 size() const { return m_length; }

    bool InRange(i32 i) const { return (u32)i < (u32)m_length; }
    bool IsEmpty() const { return m_length == 0; }
    bool IsFull() const { return m_length == capacity(); }

    const T* begin() const { return m_ptr; }
    const T* end() const { return m_ptr + m_length; }
    const T& front() const { ASSERT(InRange(0)); return m_ptr[0]; }
    const T& back() const { ASSERT(InRange(0)); return  m_ptr[m_length - 1]; }
    const T& operator[](i32 i) const { ASSERT(InRange(i)); return m_ptr[i]; }

    T* begin() { return m_ptr; }
    T* end() { return m_ptr + m_length; }
    T& front() { ASSERT(InRange(0)); return m_ptr[0]; }
    T& back() { ASSERT(InRange(0)); return m_ptr[m_length - 1]; }
    T& operator[](i32 i) { ASSERT(InRange(i)); return m_ptr[i]; }

    // ------------------------------------------------------------------------

    void Init(EAlloc allocType = EAlloc_Perm)
    {
        m_length = 0;
        SetCapacity(0);
        SetAllocator(allocType);
        m_ptr = nullptr;
    }

    void Init(EAlloc allocator, i32 cap)
    {
        Init(allocator);
        Reserve(cap);
    }

    void Reset()
    {
        pim_free(m_ptr);
        m_ptr = nullptr;
        m_length = 0;
        SetCapacity(0);
    }

    void Clear()
    {
        m_length = 0;
    }

    // ------------------------------------------------------------------------

    Slice<const T> AsCSlice() const
    {
        return { begin(), m_length };
    }

    Slice<T> AsSlice()
    {
        return { begin(), m_length };
    }

    operator Slice<const T>() const { return AsCSlice(); }
    operator Slice<T>() { return AsSlice(); }

    void Reserve(i32 newCap)
    {
        const i32 current = capacity();
        if (newCap > current)
        {
            const i32 next = Max(newCap, Max(current * 2, 16));
            m_ptr = pim_trealloc(T, GetAllocator(), m_ptr, next);
            SetCapacity(next);
        }
    }

    void ReserveRel(i32 relSize)
    {
        Reserve(m_length + relSize);
    }

    void Resize(i32 newLen)
    {
        ASSERT(newLen >= 0);
        Reserve(newLen);
        m_length = newLen;
    }

    i32 ResizeRel(i32 relSize)
    {
        const i32 prevLen = m_length;
        Resize(prevLen + relSize);
        return prevLen;
    }

    T& Grow()
    {
        const i32 iBack = m_length++;
        Reserve(iBack + 1);
        T& item = m_ptr[iBack];
        memset(&item, 0, sizeof(T));
        return item;
    }

    void Pop()
    {
        const i32 iBack = --m_length;
        ASSERT(iBack >= 0);
    }

    i32 PushBack(T item)
    {
        const i32 iBack = m_length++;
        Reserve(iBack + 1);
        m_ptr[iBack] = item;
        return iBack;
    }

    void AppendBack(T item)
    {
        const i32 iBack = m_length++;
        ASSERT(iBack < capacity());
        m_ptr[iBack] = item;
    }

    template<typename C>
    void AppendRange(const C& container)
    {
        const i32 extend = (i32)container.size();
        const i32 iDst = ResizeRel(extend);
        T* const pDst = m_ptr + iDst;
        const T* const pSrc = container.begin();
        for (i32 i = 0; i < extend; ++i)
        {
            pDst[i] = pSrc[i];
        }
    }

    T PopBack()
    {
        const i32 iBack = --m_length;
        ASSERT(iBack >= 0);
        return m_ptr[iBack];
    }

    T PopFront()
    {
        const T item = front();
        ShiftRemove(0);
        return item;
    }

    void PushFront(T item)
    {
        ShiftInsert(0, item);
    }

    void Remove(i32 idx)
    {
        const i32 iBack = --m_length;
        ASSERT(iBack >= 0);
        ASSERT((u32)idx <= (u32)iBack);

        T* const ptr = m_ptr;
        ptr[idx] = ptr[iBack];
    }

    void ShiftRemove(i32 idx)
    {
        const i32 oldBack = --m_length;
        const i32 shifts = oldBack - idx;
        ASSERT(oldBack >= 0);
        ASSERT(shifts >= 0);
        T* const ptr = m_ptr;
        memmove(ptr + idx, ptr + idx + 1, sizeof(T) * shifts);
    }

    void ShiftInsert(i32 idx, const T& value)
    {
        const i32 newBack = m_length++;
        const i32 shifts = newBack - idx;
        ASSERT(idx >= 0);
        ASSERT(shifts >= 0);
        Reserve(newBack + 1);
        T* const ptr = m_ptr;
        memmove(ptr + idx + 1, ptr + idx, sizeof(T) * shifts);
        ptr[idx] = value;
    }

    void PushBytes(const void* src, i32 bytes)
    {
        ASSERT(src);
        ASSERT(bytes >= 0);
        const i32 iBack = ResizeRel(bytes / sizeof(T));
        memcpy(m_ptr + iBack, src, bytes);
    }

    i32 Find(T key) const
    {
        const T* ptr = m_ptr;
        const i32 len = m_length;
        for (i32 i = len - 1; i >= 0; --i)
        {
            if (ptr[i] == key)
            {
                return i;
            }
        }
        return -1;
    }

    bool Contains(T key) const
    {
        return Find(key) != -1;
    }

    bool FindAdd(T value)
    {
        if (!Contains(value))
        {
            PushBack(value);
            return true;
        }
        return false;
    }

    bool FindRemove(T value)
    {
        i32 i = Find(value);
        if (i != -1)
        {
            Remove(i);
            return true;
        }
        return false;
    }
};

SASSERT(sizeof(Array<i32>) == 16);

template<typename T, typename C>
static void Copy(Array<T>& dst, const C& src)
{
    const i32 length = (i32)src.m_length;
    const i32 bytes = sizeof(T) * length;
    dst.Resize(length);
    memcpy(dst.begin(), src.begin(), bytes);
}

template<typename T>
static void Move(Array<T>& dst, Array<T>& src)
{
    dst.Reset();
    memcpy(&dst, &src, sizeof(Array<T>));
    memset(&src, 0, sizeof(Array<T>));
}

template<typename T>
static Array<T> CreateArray(EAlloc allocator = EAlloc_Perm, i32 cap = 0)
{
    Array<T> arr;
    arr.Init(allocator, cap);
    return arr;
}
