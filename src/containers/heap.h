#pragma once

#include "common/macro.h"
#include "common/minmax.h"
#include "containers/array.h"

template<typename T>
struct Heap
{
    i32 size() const { return m_items.size(); }
    i32 capacity() const { return m_items.capacity(); }

    void Init(AllocType allocator, i32 minCap)
    {
        m_items.Init(allocator, minCap);
    }
    void Reset()
    {
        m_items.Reset();
    }
    void Clear()
    {
        m_items.Clear();
    }

    void Insert(const T& item)
    {
        HeapUp((u32)m_items.PushBack(item));
    }

    bool Remove(T& largest)
    {
        const i32 len = m_items.size();
        if (len > 0)
        {
            largest = m_items[0];
            m_items[0] = m_items.back();
            m_items.Pop();
            if (len > 1)
            {
                HeapDown(0);
            }
            return true;
        }
        return false;
    }

    bool RemoveBestFit(const T& key, T& result)
    {
        const u32 len = (u32)m_items.size();
        T* items = m_items.begin();
        u32 i = 0;
    loop:
        u32 iLeft = GetLeftChild(i);
        u32 iRight = GetRightChild(i);
        if (iRight < len)
        {
            u32 iChild = items[iLeft] < items[iRight] ? iLeft : iRight;
            if (key < items[iChild])
            {
                i = iChild;
                goto loop;
            }
        }
        if (i < len)
        {
            if (key < items[i])
            {
                result = items[i];
                items[i] = items[len - 1];
                m_items.Pop();
                HeapDown(i);
                return true;
            }
        }
        return false;
    }

private:
    Array<T> m_items;

    static u32 GetLeftChild(u32 i)
    {
        return (i << 1u) + 1u;
    }
    static u32 GetRightChild(u32 i)
    {
        return (i << 1u) + 2u;
    }
    static u32 GetParent(u32 i)
    {
        return (i - 1u) >> 1u;
    }

    void HeapUp(u32 iChild)
    {
        T* items = m_items.begin();
    loop:
        u32 iParent = GetParent(iChild);
        if (iChild)
        {
            if (items[iParent] < items[iChild])
            {
                Swap(items[iParent], items[iChild]);
                iChild = iParent;
                goto loop;
            }
        }
    }

    void HeapDown(u32 iParent)
    {
        const u32 len = (u32)m_items.size();
        T* items = m_items.begin();
    loop:
        u32 lChild = GetLeftChild(iParent);
        u32 rChild = GetRightChild(iParent);
        u32 iChild = 0;
        if (rChild >= len)
        {
            if (lChild >= len)
            {
                return;
            }
            iChild = lChild;
        }
        else
        {
            iChild = items[lChild] < items[rChild] ? rChild : lChild;
        }
        if (items[iParent] < items[iChild])
        {
            Swap(items[iParent], items[iChild]);
            iParent = iChild;
            goto loop;
        }
    }
};
