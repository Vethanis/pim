#pragma once

#include "common/macro.h"
#include "common/int_types.h"
#include "allocator/allocator.h"
#include "os/atomics.h"
#include "os/thread.h"

struct AArray
{
    static constexpr i32 kRecordSize = 64;
    static constexpr i32 kRecordMask = kRecordSize - 1;
    SASSERT((kRecordSize & kRecordMask) == 0);

    struct Record
    {
        Record* pNext;
        isize values[kRecordSize];
    };

    static constexpr Record* kNull = nullptr;

    Record* m_head;
    i32 m_innerLength;
    i32 m_length;
    i32 m_capacity;
    EAlloc m_allocator;

    EAlloc GetAllocator() const { return (EAlloc)Load(m_allocator, MO_Relaxed); }
    i32 size() const { return Load(m_length); }
    i32 capacity() const { return Load(m_capacity); }
    bool IsEmpty() const { return size() == 0; }
    bool InRange(i32 i) const { return (u32)i < (u32)size(); }

    void Init(EAlloc allocator)
    {
        StorePtr(m_head, kNull);
        Store(m_innerLength, 0);
        Store(m_length, 0);
        Store(m_capacity, 0);
        Store((i32&)m_allocator, (i32)allocator);
    }

    void Reset()
    {
        Record* pHead = LoadPtr(m_head);
        if (pHead && CmpExStrongPtr(m_head, pHead, kNull))
        {
            Store(m_innerLength, 0);
            Store(m_length, 0);
            Store(m_capacity, 0);
            while (pHead)
            {
                Record* pNext = LoadPtr(pHead->pNext);
                CAllocator.Free(pHead);
                pHead = pNext;
            }
        }
        ASSERT(!pHead);
    }

    void Clear()
    {
        Store(m_innerLength, 0);
        Store(m_length, 0);
    }

    Record* RecordAt(i32 i)
    {
        ASSERT(i >= 0);
        Record* pRecord = LoadPtr(m_head);
        while (i >= kRecordSize)
        {
            ASSERT(pRecord);
            pRecord = LoadPtr(pRecord->pNext);
            i -= kRecordSize;
        }
        ASSERT(pRecord);
        return pRecord;
    }

    isize& RefAt(i32 i)
    {
        return RecordAt(i)->values[i & kRecordMask];
    }

    void* LoadAt(i32 i)
    {
        return (void*)Load(RefAt(i));
    }

    void StoreAt(i32 i, void* value)
    {
        Store(RefAt(i), (isize)value);
    }

    void* ExchangeAt(i32 i, void* value)
    {
        return (void*)Exchange(RefAt(i), (isize)value);
    }

    void Reserve(i32 minSize)
    {
        ASSERT(minSize >= 0);
        while (capacity() < minSize)
        {
            u64 spins = 0;
            Record* pNew = Allocator::CallocT<Record>(GetAllocator(), 1);
            Record** ppNext = &m_head;
            Record* pNext = LoadPtr(*ppNext);
        findtail:
            while (pNext)
            {
                ppNext = &(pNext->pNext);
                pNext = LoadPtr(*ppNext);
            }
            if (CmpExStrongPtr(*ppNext, pNext, pNew))
            {
                FetchAdd(m_capacity, kRecordSize);
            }
            else
            {
                OS::Spin(++spins);
                goto findtail;
            }
        }
    }

    void PushBack(void* value)
    {
        const i32 i = Inc(m_innerLength);
        Reserve(i + 1);
        StoreAt(i, value);
        Inc(m_length, MO_AcqRel);
    }

    void* TryPopBack()
    {
        i32 len = Load(m_length);
        if (len > 0 && CmpExStrong(m_length, len, len - 1))
        {
            void* ptr = LoadAt(len - 1);
            Dec(m_innerLength);
            return ptr;
        }
        return nullptr;
    }

    void* TryRemoveAt(i32 i)
    {
        void* backVal = TryPopBack();
        if (backVal && i < size())
        {
            return ExchangeAt(i, backVal);
        }
        return backVal;
    }
};
