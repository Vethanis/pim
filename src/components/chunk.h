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

struct GenId
{
    i32 index;
    i32 version;
};

struct Entity
{
    GenId id;
};

struct ComponentRow
{
    mutable OS::RWLock m_ptrLock;
    OS::Mutex m_freeLock;
    OS::RWFlag* m_flags;
    i32* m_versions;
    u8* m_data;
    i32 m_innerLength;
    i32 m_length;
    i32 m_stride;
    i32 m_capacity;
    Queue<GenId> m_free;

    void Init(TypeId type)
    {
        memset(this, 0, sizeof(*this));
        m_stride = type.SizeOf();
        m_ptrLock.Open();
        m_freeLock.Open();
        m_free.Init(Alloc_Pool);
    }

    void Reset()
    {
        m_ptrLock.LockWriter();
        m_freeLock.Lock();
        Allocator::Free(m_flags);
        Allocator::Free(m_data);
        Allocator::Free(m_versions);
        m_free.Reset();
        m_ptrLock.Close();
        m_freeLock.Close();
        memset(this, 0, sizeof(*this));
    }

    void _LockRead(i32 i)
    {
        u64 spins = 0;
        while (!m_flags[i].TryLockReader())
        {
            OS::Spin(++spins * 100);
        }
    }

    void _UnlockRead(i32 i)
    {
        m_flags[i].UnlockReader();
    }

    void _LockWrite(i32 i)
    {
        u64 spins = 0;
        while (!m_flags[i].TryLockWriter())
        {
            OS::Spin(++spins * 100);
        }
    }

    void _UnlockWrite(i32 i)
    {
        m_flags[i].UnlockWriter();
    }

    bool InRange(i32 i) const { return (u32)i < (u32)Load(m_length); }

    bool _IsCurrent(GenId id) const
    {
        return InRange(id.index) && (Load(m_versions[id.index]) == id.version);
    }
    bool IsCurrent(GenId id) const
    {
        OS::ReadGuard guard(m_ptrLock);
        return _IsCurrent(id);
    }

    GenId Alloc()
    {
        GenId id;
        m_freeLock.Lock();
        bool popped = m_free.TryPop(id);
        m_freeLock.Unlock();

        if (!popped)
        {
            const i32 back = Inc(m_innerLength);
            id.index = back;
            id.version = 1;
            if (back >= Load(m_capacity))
            {
                m_ptrLock.LockWriter();
                if (back >= Load(m_capacity))
                {
                    const i32 cap = Max(Load(m_capacity) * 2, 64);
                    m_flags = Allocator::ReallocT<OS::RWFlag>(Alloc_Pool, m_flags, cap);
                    m_data = (u8*)Allocator::Realloc(Alloc_Pool, m_data, cap * m_stride);
                    m_versions = Allocator::ReallocT<i32>(Alloc_Pool, m_versions, cap);
                    Store(m_capacity, cap);
                }
                m_ptrLock.UnlockWriter();
            }

            m_ptrLock.LockReader();
            Store(m_versions[back], 1);
            Store(m_flags[back].m_state, 0);
            Inc(m_length);
            goto inLock;
        }

        m_ptrLock.LockReader();
    inLock:
        _LockWrite(id.index);
        memset(m_data + id.index * m_stride, 0, m_stride);
        _UnlockWrite(id.index);
        m_ptrLock.UnlockReader();

        return id;
    }

    bool Free(GenId id)
    {
        m_ptrLock.LockReader();
        bool freed = InRange(id.index) && CmpExStrong(m_versions[id.index], id.version, id.version + 1);
        m_ptrLock.UnlockReader();
        if (freed)
        {
            m_freeLock.Lock();
            m_free.Push(id);
            m_freeLock.Unlock();
        }
        return freed;
    }

    bool Read(GenId id, void* dst)
    {
        ASSERT(dst);
        OS::ReadGuard ptrGuard(m_ptrLock);
        if (_IsCurrent(id))
        {
            const void* src = m_data + id.index * m_stride;
            _LockRead(id.index);
            memcpy(dst, src, m_stride);
            _UnlockRead(id.index);
            return true;
        }
        return false;
    }

    bool Write(GenId id, const void* src)
    {
        ASSERT(src);
        OS::ReadGuard ptrGuard(m_ptrLock);
        if (_IsCurrent(id))
        {
            void* dst = m_data + id.index * m_stride;
            _LockWrite(id.index);
            memcpy(dst, src, m_stride);
            _UnlockWrite(id.index);
        }
        return false;
    }

    static ComponentRow& Get(TypeId id)
    {
        TypeData& rData = id.AsData();
        ComponentRow* pRow = LoadPtr(rData.pRow);
        if (pRow)
        {
            return *pRow;
        }
        pRow = Allocator::AllocT<ComponentRow>(Alloc_Pool, 1);
        pRow->Init(id);
        ComponentRow* pOld = nullptr;
        if (CmpExStrongPtr(rData.pRow, pOld, pRow))
        {
            return *pRow;
        }
        else
        {
            pRow->Reset();
            Allocator::Free(pRow);
            return *LoadPtr(rData.pRow);
        }
    }
};

struct ComponentId
{
    TypeId type;
    GenId id;

    bool IsCurrent() const
    {
        return ComponentRow::Get(type).IsCurrent(id);
    }
    bool Read(void* dst)
    {
        return ComponentRow::Get(type).Read(id, dst);
    }
    bool Write(const void* src)
    {
        return ComponentRow::Get(type).Write(id, src);
    }
};

struct ComponentIds
{
    mutable OS::RWLock m_lock;
    Array<ComponentId> m_ids;

    void Init()
    {
        m_lock.Open();
        m_ids.Init(Alloc_Pool);
    }

    void Reset()
    {
        m_lock.LockWriter();
        for (ComponentId id : m_ids)
        {
            ComponentRow& row = ComponentRow::Get(id.type);
            row.Free(id.id);
        }
        m_ids.Reset();
        m_lock.Close();
    }

    i32 _Find(TypeId type) const
    {
        ASSERT(!type.IsNull());
        for (i32 i = 0, c = m_ids.size(); i < c; ++i)
        {
            if (m_ids[i].type == type)
            {
                return i;
            }
        }
        return -1;
    }

    bool Has(TypeId type) const
    {
        OS::ReadGuard guard(m_lock);
        return _Find(type) != -1;
    }

    template<typename T>
    bool Has() const
    {
        return Has(TypeOf<T>());
    }

    bool Get(TypeId type, ComponentId& result) const
    {
        OS::ReadGuard guard(m_lock);
        i32 i = _Find(type);
        if (i != -1)
        {
            result = m_ids[i];
            return true;
        }
        result = {};
        return false;
    }

    ComponentId GetAdd(TypeId type)
    {
        ComponentId result;
        if (Get(type, result))
        {
            return result;
        }
        ComponentRow& row = ComponentRow::Get(type);
        ComponentId id;
        id.type = type;
        id.id = row.Alloc();
        OS::WriteGuard guard(m_lock);
        i32 i = _Find(type);
        if (i != -1)
        {
            row.Free(id.id);
            return m_ids[i];
        }
        m_ids.PushBack(id);
        return id;
    }

    template<typename T>
    ComponentId GetAdd()
    {
        return GetAdd(TypeOf<T>());
    }

    bool Remove(TypeId type)
    {
        if (Has(type))
        {
            GenId id;
            {
                OS::WriteGuard guard(m_lock);
                i32 i = _Find(type);
                if (i == -1)
                {
                    return false;
                }
                id = m_ids[i].id;
                m_ids.Remove(i);
            }
            return ComponentRow::Get(type).Free(id);
        }
        return false;
    }

    template<typename T>
    bool Remove()
    {
        return Remove(TypeOf<T>());
    }
};

struct Entities
{
    mutable OS::RWLock m_lock;
    Array<i32> m_versions;
    Array<ComponentIds*> m_ids;

    OS::Mutex m_freeLock;
    Queue<Entity> m_free;
};
