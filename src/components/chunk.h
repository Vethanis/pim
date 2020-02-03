#pragma once

#include "common/typeid.h"
#include "common/round.h"
#include "containers/array.h"
#include "os/thread.h"
#include "os/atomics.h"

struct Entity
{
    i32 index;
    i32 version;

    bool IsNull() const { return (index | version) == 0; }
    bool IsNotNull() const { return !IsNull(); }
    u64 AsQword() const
    {
        u64 x = (u32)index;
        x <<= 32;
        x |= (u32)version;
        return x;
    }

    bool operator==(Entity rhs) const
    {
        return ((index - rhs.index) | (version - rhs.version)) == 0;
    }
    bool operator!=(Entity rhs) const
    {
        return ((index - rhs.index) | (version - rhs.version)) != 0;
    }
    bool operator<(Entity rhs) const
    {
        return AsQword() < rhs.AsQword();
    }
};

struct Chunk
{
    static constexpr i32 kAlign = 64;
    static constexpr i32 kAlignMask = kAlign - 1;

    struct Row
    {
        OS::RWLock lock;
        void* ptr;
    };

    OS::RWLock m_lock;
    i32 m_rowCount;
    const TypeId* m_types;
    Row* m_rows;
    i32 m_length;
    i32 m_capacity;

    static constexpr i32 Align(i32 x)
    {
        return x + ((kAlign - (x & kAlignMask)) & kAlignMask);
    }

    static void* Align(void* p)
    {
        isize x = (isize)p;
        x = x + ((kAlign - (x & kAlignMask)) & kAlignMask);
        return (void*)x;
    }

    static Chunk* Create(Slice<TypeId> types, i32 cap)
    {
        cap = Align(cap);

        const i32 rowCount = types.size();
        constexpr i32 chunkBytes = Align(sizeof(Chunk));
        const i32 typesBytes = Align(sizeof(TypeId) * rowCount);
        const i32 rowBytes = Align(sizeof(Row) * rowCount);
        constexpr i32 typesOffset = 0 + chunkBytes;
        const i32 rowsOffset = typesOffset + typesBytes;
        const i32 dataOffset = rowsOffset + rowBytes;

        i32 bytes = 0;
        bytes += chunkBytes;
        bytes += typesBytes;
        bytes += rowBytes;
        for (TypeId type : types)
        {
            bytes += Align(type.SizeOf() * cap);
        }

        Chunk* pChunk = (Chunk*)Allocator::Calloc(Alloc_Pool, bytes);
        pChunk->m_capacity = cap;
        pChunk->m_length = 0;
        pChunk->m_lock.Open();
        pChunk->m_rowCount = rowCount;

        u8* pBase = (u8*)pChunk;
        TypeId* pTypes = (TypeId*)(pBase + typesOffset);
        memcpy(pTypes, types.begin(), types.bytes());
        pChunk->m_types = pTypes;

        Row* pRows = (Row*)(pBase + rowsOffset);
        pChunk->m_rows = pRows;
        u8* pData = pBase + dataOffset;
        for (i32 i = 0; i < rowCount; ++i)
        {
            pRows[i].lock.Open();
            pRows[i].ptr = pData;
            pData += Align(types[i].SizeOf() * cap);
        }

        return pChunk;
    }

    static void Destroy(Chunk* pChunk)
    {
        if (!pChunk)
        {
            return;
        }
        pChunk->m_lock.LockWriter();
        const i32 rowCount = pChunk->m_rowCount;
        Row* pRows = pChunk->m_rows;
        for (i32 i = 0; i < rowCount; ++i)
        {
            pRows[i].lock.LockWriter();
            pRows[i].lock.Close();
        }
        pChunk->m_lock.Close();
        Allocator::Free(pChunk);
    }

    i32 FindType(TypeId type) const
    {
        const i32 rowCount = m_rowCount;
        const TypeId* types = m_types;
        for (i32 i = 0; i < rowCount; ++i)
        {
            if (types[i] == type)
            {
                return i;
            }
        }
        return -1;
    }

    bool HasType(TypeId type) const
    {
        return FindType(type) != -1;
    }

    template<typename T>
    Slice<const T> BorrowR(TypeId type)
    {
        m_lock.LockReader();
        i32 i = FindType(type);
        ASSERT(i != -1);
        m_rows[i].lock.LockReader();
        return { (const T*)m_rows[i].ptr, Load(m_length) };
    }

    template<typename T>
    Slice<T> BorrowRW(TypeId type)
    {
        m_lock.LockReader();
        i32 i = FindType(type);
        ASSERT(i != -1);
        m_rows[i].lock.LockWriter();
        return { (const T*)m_rows[i].ptr, Load(m_length) };
    }

    template<typename T>
    void ReturnR(Slice<const T> loan)
    {
        i32 i = FindType(TypeOf<T>());
        ASSERT(i != -1);
        m_rows[i].lock.UnlockReader();
        m_lock.UnlockReader();
    }

    template<typename T>
    void ReturnRW(Slice<const T> loan)
    {
        i32 i = FindType(TypeOf<T>());
        ASSERT(i != -1);
        m_rows[i].lock.UnlockWriter();
        m_lock.UnlockReader();
    }
};

struct Chunks
{
    OS::RWLock m_lock;
    Array<Chunk*> m_chunks;
};

struct Entities
{
    OS::RWLock m_lock;
    Array<i32> m_versions;
    Array<Chunk*> m_chunks;
    Array<i32> m_offsets;
};
