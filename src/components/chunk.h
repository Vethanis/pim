#pragma once

#include "common/typeid.h"
#include "common/round.h"
#include "common/sort.h"
#include "common/hash.h"
#include "common/guid_util.h"
#include "containers/mtarray.h"
#include "containers/atomic_array.h"
#include "containers/mtqueue.h"
#include "containers/hash_dict.h"
#include "containers/obj_table.h"
#include "os/thread.h"
#include "os/atomics.h"

struct Entity
{
    i32 index;
    i32 version;
};

struct ComponentRow
{
    mutable OS::RWLock m_lock;
    i32* m_versions;
    isize* m_ptrs;
    OS::RWFlag* m_flags;
    i32 m_stride;
    i32 m_innerLength;
    i32 m_length;
    i32 m_capacity;
    ChunkAllocator m_allocator;

    ComponentRow()
    {
        m_lock.Open();
        m_capacity = ChunkAllocator::kChunkSize;
        m_versions = Allocator::CallocT<i32>(Alloc_Pool, m_capacity);
        m_ptrs = Allocator::CallocT<isize>(Alloc_Pool, m_capacity);
        m_flags = Allocator::CallocT<OS::RWFlag>(Alloc_Pool, m_capacity);
    }

    ~ComponentRow()
    {
        m_lock.LockWriter();
        Allocator::Free(m_versions);
        Allocator::Free(m_ptrs);
        Allocator::Free(m_flags);
        m_allocator.Reset();
        m_lock.Close();
    }

    void Init(TypeId type)
    {
        m_stride = type.SizeOf();
        m_allocator.Init(Alloc_Pool, m_stride);
    }

    i32 size() const
    {
        return Load(m_length, MO_Relaxed);
    }
    i32 capacity() const
    {
        return Load(m_capacity, MO_Relaxed);
    }

    bool InRange(i32 i) const
    {
        return (u32)i < (u32)size();
    }

    bool _IsCurrent(Entity entity) const
    {
        ASSERT(InRange(entity.index));
        return Load(m_versions[entity.index], MO_Relaxed) == entity.version;
    }
    bool IsCurrent(Entity entity) const
    {
        OS::ReadGuard guard(m_lock);
        return _IsCurrent(entity);
    }

    void OnGrow(i32 length)
    {
        if (length > capacity())
        {
            OS::WriteGuard guard(m_lock);
            const i32 curCap = capacity();
            if (length > curCap)
            {
                const i32 newCap = Max(length, Max(curCap * 2, ChunkAllocator::kChunkSize));
                m_versions = Allocator::ReallocT<i32>(Alloc_Pool, m_versions, newCap);
                m_ptrs = Allocator::ReallocT<isize>(Alloc_Pool, m_ptrs, newCap);
                m_flags = Allocator::ReallocT<OS::RWFlag>(Alloc_Pool, m_flags, newCap);
                Store(m_capacity, newCap);
            }
        }
        OS::ReadGuard guard(m_lock);
        i32 prev = Load(m_innerLength);
        while (prev < length)
        {
            if (CmpExStrong(m_innerLength, prev, prev + 1))
            {
                Store(m_versions[prev], 0);
                Store(m_ptrs[prev], 0);
                Store(m_flags[prev].m_state, 0);
                Inc(m_length);
            }
        }
    }

    void Read(Entity entity, void* dst)
    {
        ASSERT(dst);
        OS::ReadGuard guard(m_lock);
        ASSERT(_IsCurrent(entity));
        const void* src = (const void*)Load(m_ptrs[entity.index], MO_Relaxed);
        ASSERT(src);
        m_flags[entity.index].LockReader();
        memcpy(dst, src, m_stride);
        m_flags[entity.index].UnlockReader();
    }

    void Write(Entity entity, const void* src)
    {
        ASSERT(src);
        OS::ReadGuard guard(m_lock);
        ASSERT(_IsCurrent(entity));
        void* dst = (void*)Load(m_ptrs[entity.index], MO_Relaxed);
        ASSERT(dst);
        m_flags[entity.index].LockWriter();
        memcpy(dst, src, m_stride);
        m_flags[entity.index].UnlockWriter();
    }

    bool Add(Entity entity)
    {
        const i32 i = entity.index;
        ASSERT(InRange(i));

        void* pData = m_allocator.Allocate();
        bool added = false;

        m_lock.LockReader();
        i32 prevVersion = Load(m_versions[i]);
        bool newer = (entity.version - prevVersion) > 0;
        if (newer && CmpExStrong(m_versions[i], prevVersion, entity.version))
        {
            isize prevPtr = 0;
            added = CmpExStrong(m_ptrs[i], prevPtr, (isize)pData);
            ASSERT(added);
        }
        m_lock.UnlockReader();

        if (!added)
        {
            m_allocator.Free(pData);
        }

        return added;
    }

    bool Remove(Entity entity)
    {
        const i32 i = entity.index;
        ASSERT(InRange(i));
        OS::ReadGuard guard(m_lock);
        i32 prev = entity.version;
        if (CmpExStrong(m_versions[i], prev, prev + 1))
        {
            isize prevPtr = Load(m_ptrs[i]);
            ASSERT(prevPtr);
            if (CmpExStrong(m_ptrs[i], prevPtr, 0))
            {
                m_allocator.Free((void*)prevPtr);
            }
            else
            {
                ASSERT(false);
            }
            return true;
        }
        return false;
    }
};

struct TypeIdComparator
{
    static bool Equals(const TypeId& lhs, const TypeId& rhs)
    {
        return lhs.handle == rhs.handle;
    }
    static i32 Compare(const TypeId& lhs, const TypeId& rhs)
    {
        if (lhs.handle != rhs.handle)
        {
            return lhs.handle < rhs.handle ? -1 : 1;
        }
        return 0;
    }
    static u32 Hash(const TypeId& id)
    {
        return Fnv32Qword(id.handle);
    }
    static constexpr Comparator<TypeId> Value = { Equals, Compare, Hash };
};

struct Table
{
    using Rows = ObjTable<TypeId, ComponentRow, TypeIdComparator::Value>;

    MtQueue<Entity> m_freelist;
    MtArray<i32> m_versions;
    Rows m_rows;

    ComponentRow* GetRow(TypeId type)
    {
        ComponentRow* pRow = m_rows.Get(type);
        if (!pRow)
        {
            pRow = m_rows.New();
            pRow->Init(type);
            if (!m_rows.Add(type, pRow))
            {
                m_rows.Delete(pRow);
                pRow = m_rows.Get(type);
                ASSERT(pRow);
            }
        }
        return pRow;
    }

    bool IsCurrent(Entity entity) const
    {
        return m_versions.Read(entity.index) == entity.version;
    }

    Entity Create()
    {
        Entity entity;
        if (!m_freelist.TryPop(entity))
        {
            entity.version = 1;
            entity.index = m_versions.PushBack(entity.version);
            const i32 len = entity.index + 1;
            for (auto pair : m_rows.BeginIter())
            {
                pair.Value->OnGrow(len);
            }
            m_rows.EndIter();
        }
        return entity;
    }

    bool Destroy(Entity entity)
    {
        i32 version = entity.version;
        if (m_versions.CmpEx(entity.index, version, version + 2))
        {
            for (auto pair : m_rows.BeginIter())
            {
                pair.Value->Remove(entity);
            }
            m_rows.EndIter();
            entity.version += 2;
            m_freelist.Push(entity);
            return true;
        }
        return false;
    }

    bool Add(Entity entity, TypeId type)
    {
        if (IsCurrent(entity))
        {
            return GetRow(type)->Add(entity);
        }
        return false;
    }

    bool Remove(Entity entity, TypeId type)
    {
        if (IsCurrent(entity))
        {
            return GetRow(type)->Remove(entity);
        }
        return false;
    }
};
