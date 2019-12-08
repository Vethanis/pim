#pragma once

#include "common/macro.h"
#include "common/int_types.h"
#include "allocator/allocator.h"
#include "containers/slice.h"
#include "common/memory.h"

template<typename T>
struct Array
{
    T* m_ptr;
    i32 m_length;
    i32 m_capacity;
    AllocType m_allocType;

    inline bool InRange(i32 i) const { return (u32)i < (u32)m_length; }
    inline i32 Size() const { return m_length; }
    inline i32 Capacity() const { return m_capacity; }
    inline i32 SizeBytes() const { return m_length * sizeof(T); }
    inline i32 CapacityBytes() const { return m_capacity * sizeof(T); }
    inline bool IsEmpty() const { return m_length == 0; }
    inline bool IsFull() const { return m_length == m_capacity; }

    inline const T* begin() const { return m_ptr; }
    inline const T* end() const { return m_ptr + m_length; }
    inline const T& front() const { Assert(InRange(0)); return m_ptr[0]; }
    inline const T& back() const { Assert(InRange(0)); return  m_ptr[m_length - 1]; }
    inline const T& operator[](i32 i) const { Assert(InRange(i)); return m_ptr[i]; }

    inline T* begin() { return m_ptr; }
    inline T* end() { return m_ptr + m_length; }
    inline T& front() { Assert(InRange(0)); return m_ptr[0]; }
    inline T& back() { Assert(InRange(0)); return m_ptr[m_length - 1]; }
    inline T& operator[](i32 i) { Assert(InRange(i)); return m_ptr[i]; }

    inline Slice<const T> AsSlice() const
    {
        return { begin(), Size() };
    }
    inline Slice<T> AsSlice()
    {
        return { begin(), Size() };
    }

    inline operator Slice<const T>() const { return AsSlice(); }
    inline operator Slice<T>() { return AsSlice(); }

    template<typename U>
    inline Slice<const U> Cast() const
    {
        return { reinterpret_cast<const U*>(begin()), SizeBytes() / (i32)sizeof(U) };
    }
    template<typename U>
    inline Slice<U> Cast()
    {
        return { reinterpret_cast<U*>(begin()), SizeBytes() / (i32)sizeof(U) };
    }

    inline void Init(AllocType allocType)
    {
        m_ptr = nullptr;
        m_length = 0;
        m_capacity = 0;
        m_allocType = allocType;
    }
    inline void Clear()
    {
        m_length = 0;
    }
    inline void Reset()
    {
        AllocationT<T> alloc = { m_ptr, m_capacity, m_allocType };
        Allocator::FreeT(alloc);
        m_ptr = nullptr;
        m_capacity = 0;
        m_length = 0;
    }
    inline void Reserve(i32 newCap)
    {
        AllocationT<T> alloc = { m_ptr, m_capacity, m_allocType };
        Allocator::ReserveT(alloc, newCap);
        m_ptr = alloc.slice.ptr;
        m_capacity = alloc.slice.len;
    }
    inline void ReserveRel(i32 relSize)
    {
        Reserve(m_length + relSize);
    }
    inline void Resize(i32 newLen)
    {
        Reserve(newLen);
        m_length = newLen;
    }
    inline i32 ResizeRel(i32 relSize)
    {
        i32 prevLen = m_length;
        Resize(m_length + relSize);
        return prevLen;
    }
    inline T& Grow()
    {
        const i32 iBack = m_length;
        const i32 newLen = iBack + 1;
        Reserve(newLen);
        m_length = newLen;
        T& item = m_ptr[iBack];
        MemClear(&item, sizeof(T));
        return item;
    }
    inline void Pop()
    {
        Assert(m_length > 0);
        --m_length;
    }
    inline T PopValue()
    {
        T item = back();
        Pop();
        return item;
    }
    inline T PopFront()
    {
        T* ptr = m_ptr;
        const i32 len = m_length--;
        Assert(len > 0);
        T item = ptr[0];
        for (i32 i = 1; i < len; ++i)
        {
            ptr[i - 1] = ptr[i];
        }
        return item;
    }
    inline void Remove(i32 i)
    {
        T* ptr = m_ptr;
        const i32 b = --m_length;
        Assert((u32)i <= (u32)b);
        ptr[i] = ptr[b];
    }
    inline void ShiftRemove(i32 idx)
    {
        Assert((u32)idx < (u32)m_length);
        T* ptr = begin();
        const i32 len = m_length--;
        for (i32 i = idx + 1; i < len; ++i)
        {
            ptr[i - 1] = ptr[i];
        }
    }
    inline void ShiftInsert(i32 idx, const T& value)
    {
        const i32 len = ++m_length;
        Assert((u32)idx < (u32)len);
        Reserve(len);
        T* ptr = m_ptr;
        for (i32 i = len - 1; i > idx; --i)
        {
            ptr[i] = ptr[i - 1];
        }
        ptr[idx] = value;
    }
    inline void ShiftTail(i32 tailSize)
    {
        Assert(tailSize >= 0);
        T* ptr = m_ptr;
        const i32 len = m_length;
        const i32 diff = len - tailSize;
        if (diff > 0)
        {
            for (i32 i = diff; i < len; ++i)
            {
                ptr[i - diff] = ptr[i];
            }
            m_length = tailSize;
        }
    }
    inline i32 Find(const T& value) const
    {
        const T* ptr = m_ptr;
        const i32 len = m_length;
        for (i32 i = 0; i < len; ++i)
        {
            if (MemEq(ptr + i, &value, sizeof(T)))
            {
                return i;
            }
        }
        return -1;
    }
    inline i32 RFind(const T& value) const
    {
        const T* ptr = m_ptr;
        const i32 len = m_length;
        for (i32 i = len - 1; i >= 0; --i)
        {
            if (MemEq(ptr + i, &value, sizeof(T)))
            {
                return i;
            }
        }
        return -1;
    }
    inline bool FindRemove(const T& value)
    {
        const i32 idx = RFind(value);
        if (idx != -1)
        {
            Remove(idx);
            return true;
        }
        return false;
    }
    inline bool UniquePush(const T& value)
    {
        const i32 idx = RFind(value);
        if (idx == -1)
        {
            Grow() = value;
            return true;
        }
        return false;
    }
    inline void Copy(Slice<const T> other)
    {
        Resize(other.Size());
        MemCpy(begin(), other.begin(), sizeof(T) * Size());
    }
    inline void Copy(Slice<T> other)
    {
        Resize(other.Size());
        MemCpy(begin(), other.begin(), sizeof(T) * Size());
    }
    inline void Assume(Array<T>& other)
    {
        Reset();
        MemCpy(this, &other, sizeof(*this));
        MemClear(&other, sizeof(*this));
    }

    inline void* At(i32 i)
    {
        Assert(InRange(i));
        return begin() + i;
    }

    template<typename U>
    inline U* As(i32 i)
    {
        Assert((u32)(i * sizeof(U)) < (u32)SizeBytes());
        return reinterpret_cast<U*>(begin()) + i;
    }

    template<typename U>
    inline U& Embed()
    {
        Assert(sizeof(T) == 1);
        i32 prevSize = ResizeRel(sizeof(U));
        U* pU = reinterpret_cast<U*>(At(prevSize));
        MemClear(pU, sizeof(U));
        return *pU;
    }
};

template<typename T, i32 t_capacity>
struct FixedArray
{
    i32 m_len;
    T m_ptr[t_capacity];

    inline bool InRange(i32 i) const { return (u32)i < (u32)m_len; }
    inline i32 Size() const { return m_len; }
    inline i32 Capacity() const { return t_capacity; }
    inline i32 SizeBytes() const { return m_len * sizeof(T); }
    inline i32 CapacityBytes() const { return t_capacity * sizeof(T); }
    inline bool IsEmpty() const { return m_len == 0; }
    inline bool IsFull() const { return m_len == t_capacity; }

    inline const T* begin() const { return m_ptr; }
    inline const T* end() const { return m_ptr + m_len; }
    inline const T& front() const { Assert(m_len); return m_ptr[0]; }
    inline const T& back() const { Assert(m_len); return m_ptr[m_len - 1]; }
    inline const T& operator[](i32 i) const { Assert(i < m_len); return m_ptr[i]; }

    inline T* begin() { return m_ptr; }
    inline T* end() { return m_ptr + m_len; }
    inline T& front() { Assert(m_len); return m_ptr[0]; }
    inline T& back() { Assert(m_len); return m_ptr[m_len - 1]; }
    inline T& operator[](i32 i) { Assert(i < m_len); return m_ptr[i]; }

    inline operator Slice<T>() { return { begin(), Size() }; }
    inline operator Slice<const T>() const { return { begin(), Size() }; }

    inline void Init() { m_len = 0; }
    inline void Clear() { m_len = 0; }
    inline void Reset() { m_len = 0; }
    inline void Resize(i32 newLen)
    {
        Assert(newLen < t_capacity);
        m_len = newLen;
    }
    inline T& Grow()
    {
        Assert(m_len < t_capacity);
        T& item = m_ptr[m_len++];
        MemClear(&item, sizeof(T));
        return item;
    }
    inline void Pop()
    {
        Assert(m_len);
        --m_len;
    }
    inline T PopFront()
    {
        T* ptr = begin();
        const i32 len = m_len--;
        Assert(len > 0);
        T item = ptr[0];
        for (i32 i = 1; i < len; ++i)
        {
            ptr[i - 1] = ptr[i];
        }
        return item;
    }
    inline void Remove(i32 idx)
    {
        Assert((u32)idx < (u32)m_len);
        m_ptr[idx] = m_ptr[--m_len];
    }
    inline void ShiftRemove(i32 idx)
    {
        Assert((u32)idx < (u32)m_len);
        T* ptr = begin();
        const i32 len = m_len--;
        for (i32 i = idx + 1; i < len; ++i)
        {
            ptr[i - 1] = ptr[i];
        }
    }
    inline void ShiftInsert(i32 idx, const T& value)
    {
        Assert(m_len < t_capacity);
        const i32 len = ++m_len;
        Assert(idx < len);
        T* ptr = m_ptr;
        for (i32 i = len - 1; i > idx; --i)
        {
            ptr[i] = ptr[i - 1];
        }
        ptr[idx] = value;
    }
    inline void ShiftTail(i32 tailSize)
    {
        T* ptr = m_ptr;
        const i32 len = m_len;
        const i32 diff = len - tailSize;
        if (diff > 0)
        {
            for (i32 i = diff; i < len; ++i)
            {
                ptr[i - diff] = ptr[i];
            }
            m_len = tailSize;
        }
    }
    inline i32 Find(const T& value) const
    {
        const T* ptr = m_ptr;
        const i32 len = m_len;
        for (i32 i = 0; i < len; ++i)
        {
            if (ptr[i] == value)
            {
                return i;
            }
        }
        return -1;
    }
    inline i32 RFind(const T& value) const
    {
        const T* ptr = m_ptr;
        const i32 len = m_len;
        for (i32 i = len - 1; i >= 0; --i)
        {
            if (ptr[i] == value)
            {
                return i;
            }
        }
        return -1;
    }
    inline bool FindRemove(const T& value)
    {
        const i32 idx = RFind(value);
        if (idx != -1)
        {
            Remove(idx);
            return true;
        }
        return false;
    }
    inline bool UniquePush(const T& value)
    {
        const i32 idx = RFind(value);
        if (idx == -1)
        {
            Grow() = value;
            return true;
        }
        return false;
    }
};
