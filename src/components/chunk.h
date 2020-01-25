#pragma once

#include "common/typeid.h"
#include "common/round.h"
#include "common/sort.h"
#include "common/hash.h"
#include "common/guid_util.h"
#include "containers/array.h"
#include "containers/atomic_array.h"
#include "containers/queue.h"
#include "containers/hash_dict.h"
#include "containers/obj_table.h"
#include "os/thread.h"
#include "os/atomics.h"

struct Row;
struct Chunk;
struct Chunks;

struct Entity
{
    i32 index;
    i32 version;
};

struct InChunk
{
    Chunk* pChunk;
    i32 index;
};

struct Row
{
    mutable OS::RWLock lock;
    u8* ptr;
    i32 stride;

    u8* operator[](i32 i) { return ptr + i * stride; }
    const u8* operator[](i32 i) const { return ptr + i * stride; }

    void LockReader() const { lock.LockReader(); }
    void UnlockReader() const { lock.UnlockReader(); }

    void LockWriter() { lock.LockWriter(); }
    void UnlockWriter() { lock.UnlockWriter(); }
};

struct Chunk
{
    Slice<TypeId> m_types;
    i32 m_length;
    i32 m_capacity;
    i32 m_rowCount;
    Row m_rows[0];

    static constexpr i32 kAlignment = 64;

    static Chunk* Create(AllocType allocator, Slice<TypeId> types, i32 capacity)
    {
        const i32 rowCount = types.size();
        const i32 structSize = MultipleOf((i32)sizeof(Chunk) + ((i32)sizeof(Row) * rowCount), kAlignment);

        i32 dataSize = 0;
        for (i32 i = 0; i < rowCount; ++i)
        {
            i32 stride = types[i].AsData().sizeOf;
            dataSize += MultipleOf(capacity * stride, kAlignment);
        }

        Chunk* pChunk = (Chunk*)Allocator::Alloc(allocator, structSize + dataSize);
        pChunk->m_types = types;
        pChunk->m_length = 0;
        pChunk->m_capacity = capacity;
        pChunk->m_rowCount = rowCount;

        Row* pRows = pChunk->m_rows;
        u8* pData = reinterpret_cast<u8*>(pChunk) + structSize;

        for (i32 i = 0; i < rowCount; ++i)
        {
            const i32 stride = types[i].AsData().sizeOf;

            pRows[i].lock.Open();
            pRows[i].ptr = pData;
            pRows[i].stride = stride;

            pData += MultipleOf(capacity * stride, kAlignment);
        }

        return pChunk;
    }

    static void Destroy(Chunk* pChunk)
    {
        if (pChunk)
        {
            const i32 rowCount = pChunk->m_rowCount;
            Row* pRows = pChunk->m_rows;
            for (i32 i = 0; i < rowCount; ++i)
            {
                pRows[i].lock.Close();
            }
            Allocator::Free(pChunk);
        }
    }

    void Clear() { Store(m_length, 0, MO_Release); }

    i32 size() const { return Load(m_length, MO_Relaxed); }

    i32 capacity() const { return m_capacity; }
    bool IsFull() const { return size() == m_capacity; }
    bool IsEmpty() const { return size() == 0; }
    bool InRange(i32 i) const { return (u32)i < (u32)size(); }

    bool TryAdd(i32& iOut)
    {
        const i32 cap = m_capacity;
        i32 len = Load(m_length, MO_Relaxed);
        while (len < cap)
        {
            if (CmpExStrong(m_length, len, len + 1, MO_Acquire, MO_Relaxed))
            {
                iOut = len;
                return true;
            }
        }
        iOut = -1;
        return false;
    }

    i32 FindRow(TypeId type) const
    {
        const TypeId* pTypes = m_types.begin();
        const i32 count = m_types.size();
        for (i32 i = 0; i < count; ++i)
        {
            if (pTypes[i].handle == type.handle)
            {
                return i;
            }
        }
        return -1;
    }

    Row& GetRow(i32 i) { return m_rows[i]; }
    Row* GetRow(TypeId type)
    {
        i32 i = FindRow(type);
        return (i == -1) ? nullptr : m_rows + i;
    }
};

struct Chunks
{
    Array<TypeId> m_types;
    AArray m_chunks;
    i32 m_chunkSize;

    Chunks() {}
    ~Chunks()
    {
        Chunk* pChunk = (Chunk*)m_chunks.TryPopBack();
        while (pChunk)
        {
            Chunk::Destroy(pChunk);
            pChunk = (Chunk*)m_chunks.TryPopBack();
        }
        m_chunks.Reset();
        m_types.Reset();
        m_chunkSize = 16;
    }

    void Init(AllocType allocator, Slice<TypeId> types, i32 startSize = 16)
    {
        m_types.Init(allocator);
        Copy(m_types, types);
        m_chunks.Init(allocator);
        m_chunkSize = startSize;
    }

    i32 size() const { return m_chunks.size(); }

    Chunk* GetChunk(i32 i) { return (Chunk*)m_chunks.LoadAt(i); }

    Array<InChunk> Add(i32 count)
    {
        ASSERT(count > 0);

        Array<InChunk> results;
        results.Init(Alloc_Stack);
        results.Reserve(count);

        while (count > 0)
        {
            const i32 chunkCount = size();
            {
                for (i32 i = chunkCount - 1; i >= 0; --i)
                {
                    Chunk* pChunk = GetChunk(i);
                    i32 j;
                    while (pChunk->TryAdd(j))
                    {
                        results.PushBack({ pChunk, j });
                        --count;
                        if (!count)
                        {
                            goto done;
                        }
                    }
                }
            }
            if (chunkCount == size())
            {
                i32 chunkSize = Load(m_chunkSize, MO_Relaxed);
                CmpExStrong(m_chunkSize, chunkSize, chunkSize * 2, MO_Release, MO_Relaxed);
                Chunk* pChunk = Chunk::Create(m_chunks.GetAllocator(), m_types, chunkSize);
                m_chunks.PushBack(pChunk);
            }
        }

    done:
        return results;
    }
};

static Guid ToGuid(Slice<TypeId> types)
{
    Sort(types.begin(), types.size(), OpComparable<TypeId>());
    const Guid id = ToGuid(types.begin(), types.bytes());
    return id;
}

struct Components
{
    ObjTable<Guid, Chunks, GuidComparator, 16> ms_table;

    Chunks* Get(Slice<TypeId> types)
    {
        const Guid id = ToGuid(types);
        Chunks* pChunks = ms_table.Get(id);
        if (!pChunks)
        {
            pChunks = ms_table.New();
            pChunks->Init(Alloc_Pool, types);
            if (!ms_table.Add(id, pChunks))
            {
                ms_table.Delete(pChunks);
                pChunks = ms_table.Get(id);
                ASSERT(pChunks);
            }
        }
        return pChunks;
    }

    Array<InChunk> Add(Slice<TypeId> types, i32 count)
    {
        Chunks* pChunks = Get(types);
        Array<InChunk> inChunks = pChunks->Add(count);
        ASSERT(inChunks.size() == count);
        return inChunks;
    }
};

struct Entities
{
    mutable OS::RWLock m_lock;
    Queue<i32> m_freelist;

    Array<i32> m_versions;
    Array<InChunk> m_inChunks;

    Components* m_components;

    bool IsCurrent(Entity entity) const
    {
        OS::ReadGuard guard(m_lock);
        return m_versions[entity.index] == entity.version;
    }

    Array<Entity> Create(Slice<TypeId> types, i32 count)
    {
        ASSERT(count > 0);

        Array<Entity> entities;
        entities.Init(Alloc_Stack);
        entities.Reserve(count);

        Array<InChunk> inChunks = m_components->Add(types, count);

        m_lock.LockWriter();
        m_freelist.Reserve(count);
        m_versions.ReserveRel(count);
        m_inChunks.ReserveRel(count);
        while (m_freelist.size() < (u32)count)
        {
            m_freelist.Push(m_versions.size());
            m_versions.PushBack(0);
            m_inChunks.PushBack({});
        }

        for (i32 i = 0; i < count; ++i)
        {
            const i32 e = m_freelist.Pop();
            m_inChunks[e] = inChunks[i];
            ++m_versions[e];
            Entity entity;
            entity.index = e;
            entity.version = m_versions[e];
            entities.PushBack(entity);
        }
        m_lock.UnlockWriter();

        inChunks.Reset();

        return entities;
    }
};
