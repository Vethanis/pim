#pragma once

#include "common/macro.h"
#include "common/int_types.h"
#include "common/minmax.h"
#include "allocator/allocator.h"
#include "containers/slice.h"
#include <string.h>

struct ArrayInts
{
public:
    void Set(AllocType allocator, i32 length, i32 capacity)
    {
        ASSERT((u32)length <= kMask);
        ASSERT((u32)capacity <= kMask);
        ASSERT((u32)allocator <= 0xffu);

        u64 a = 0xffu & allocator;
        u64 l = k28Mask & length;
        u64 c = k28Mask & capacity;
        m_value = (a << 56) | (l << 28) | c;
    }
    AllocType GetAllocator() const
    {
        return (AllocType)(0xff & (m_value >> 56));
    }
    i32 GetLength() const
    {
        return (i32)(k28Mask & (m_value >> 28));
    }
    i32 GetCapacity() const
    {
        return (i32)(k28Mask & m_value);
    }
    void SetAllocator(AllocType allocator)
    {
        Set(allocator, GetLength(), GetCapacity());
    }
    void SetLength(i32 length)
    {
        Set(GetAllocator(), length, GetCapacity());
    }
    void SetCapacity(i32 capacity)
    {
        Set(GetAllocator(), GetLength(), capacity);
    }

private:
    static constexpr u64 k28Mask = 0x0fffffffu;

    u64 m_value;
};

template<typename T>
struct alignas(16) Array
{
private:
    T* m_ptr;
    ArrayInts m_ints;

public:

    AllocType GetAllocator() const { return m_ints.GetAllocator(); }
    i32 capacity() const { return m_ints.GetCapacity(); }
    i32 size() const { return m_ints.GetLength(); }

    bool InRange(i32 i) const { return (u32)i < (u32)size(); }
    bool IsEmpty() const { return size() == 0; }
    bool IsFull() const { return size() == capacity(); }

    const T* begin() const { return m_ptr; }
    const T* end() const { return m_ptr + size(); }
    const T& front() const { ASSERT(InRange(0)); return m_ptr[0]; }
    const T& back() const { ASSERT(InRange(0)); return  m_ptr[size() - 1]; }
    const T& operator[](i32 i) const { ASSERT(InRange(i)); return m_ptr[i]; }

    T* begin() { return m_ptr; }
    T* end() { return m_ptr + size(); }
    T& front() { ASSERT(InRange(0)); return m_ptr[0]; }
    T& back() { ASSERT(InRange(0)); return m_ptr[size() - 1]; }
    T& operator[](i32 i) { ASSERT(InRange(i)); return m_ptr[i]; }

    // ------------------------------------------------------------------------

    void Init(AllocType allocType = Alloc_Perm)
    {
        m_ints.Set(allocType, 0, 0);
        m_ptr = nullptr;
    }

    void Init(AllocType allocator, i32 cap)
    {
        Init(allocator);
        Reserve(cap);
    }

    void Reset()
    {
        Allocator::Free(m_ptr);
        m_ptr = nullptr;
        m_ints.Set(m_ints.GetAllocator(), 0, 0);
    }

    void Clear()
    {
        m_ints.SetLength(0);
    }

    void Trim()
    {
        const i32 len = size();
        if (capacity() > len)
        {
            m_ptr = Allocator::ReallocT<T>(GetAllocator(), m_ptr, len);
            m_ints.SetCapacity(len);
        }
    }

    // ------------------------------------------------------------------------

    Slice<const T> AsCSlice() const
    {
        return { begin(), size() };
    }

    Slice<T> AsSlice()
    {
        return { begin(), size() };
    }

    operator Slice<const T>() const { return AsCSlice(); }
    operator Slice<T>() { return AsSlice(); }

    void Reserve(i32 newCap)
    {
        const i32 current = capacity();
        if (newCap > current)
        {
            const i32 next = Max(newCap, Max(current * 2, 16));
            m_ptr = Allocator::ReallocT<T>(GetAllocator(), m_ptr, next);
            m_ints.SetCapacity(next);
        }
    }

    void ReserveRel(i32 relSize)
    {
        Reserve(size() + relSize);
    }

    void Resize(i32 newLen)
    {
        ASSERT(newLen >= 0);
        Reserve(newLen);
        const i32 oldLen = size();
        const i32 diff = newLen - oldLen;
        if (diff > 0)
        {
            memset(m_ptr + oldLen, 0, sizeof(T) * diff);
        }
        m_ints.SetLength(newLen);
    }

    i32 ResizeRel(i32 relSize)
    {
        const i32 prevLen = m_length;
        Resize(prevLen + relSize);
        return prevLen;
    }

    T& Grow()
    {
        const i32 iBack = size();
        Reserve(iBack + 1);
        m_ints.SetLength(iBack + 1);
        T& item = m_ptr[iBack];
        memset(&item, 0, sizeof(T));
        return item;
    }

    void Pop()
    {
        const i32 iBack = size() - 1;
        ASSERT(iBack >= 0);
        m_ints.SetLength(iBack);
    }

    i32 PushBack(T item)
    {
        const i32 iBack = size();
        Reserve(iBack + 1);
        m_ints.SetLength(iBack + 1);
        m_ptr[iBack] = item;
        return iBack;
    }

    void AppendBack(T item)
    {
        const i32 iBack = size();
        ASSERT(iBack < capacity());
        m_ints.SetLength(iBack + 1);
        m_ptr[iBack] = item;
    }

    T PopBack()
    {
        const i32 iBack = size() - 1;
        ASSERT(iBack >= 0);
        m_ints.SetLength(iBack);
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
        const i32 iBack = size() - 1;
        ASSERT(iBack >= 0);
        ASSERT((u32)idx <= (u32)iBack);
        m_ints.SetLength(iBack);

        T* const ptr = m_ptr;
        ptr[idx] = ptr[iBack];
    }

    void ShiftRemove(i32 idx)
    {
        const i32 oldBack = size() - 1;
        const i32 shifts = oldBack - idx;
        ASSERT(oldBack >= 0);
        ASSERT(shifts >= 0);
        m_ints.SetLength(oldBack);
        T* const ptr = m_ptr;
        memmove(ptr + idx, ptr + idx + 1, sizeof(T) * shifts);
    }

    void ShiftInsert(i32 idx, const T& value)
    {
        const i32 newBack = size();
        const i32 shifts = newBack - idx;
        ASSERT(idx >= 0);
        ASSERT(shifts >= 0);
        Reserve(newBack + 1);
        m_ints.SetLength(newBack + 1);
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
        const i32 len = size();
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
    const i32 length = (i32)src.size();
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
static Array<T> CreateArray(AllocType allocator = Alloc_Perm, i32 cap = 0)
{
    Array<T> arr;
    arr.Init(allocator, cap);
    return arr;
}
