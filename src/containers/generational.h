#pragma once

#include "containers/genid.h"
#include "containers/mtarray.h"
#include "containers/mtqueue.h"

struct IdAllocator
{
    MtArray<u32> m_versions;
    MtQueue<GenId> m_freelist;

    // ------------------------------------------------------------------------

    void Init(AllocType type)
    {
        m_versions.Init(type);
        m_freelist.Init(type);
    }

    void Reset()
    {
        m_versions.Reset();
        m_freelist.Reset();
    }

    void Clear()
    {
        m_versions.Clear();
        m_freelist.Clear();
    }

    // ------------------------------------------------------------------------

    AllocType GetAllocator() const { return m_versions.GetAllocator(); }
    i32 capacity() const { return m_versions.capacity(); }
    i32 size() const { return m_versions.size(); }
    GenId operator[](i32 i) const { return { (u32)i, m_versions.LoadAt(i) }; }

    // ------------------------------------------------------------------------

    bool IsCurrent(GenId id) const { return id.version == m_versions.LoadAt(id.index); }

    bool LockSlot(GenId id)
    {
        ASSERT(id.version);
        return m_versions.CmpExAt(id.index, id.version, 0u);
    }

    void UnlockSlot(GenId id)
    {
        ASSERT(id.version);
        u32 prev = m_versions.ExchangeAt(id.index, id.version);
        ASSERT(!prev);
    }

    GenId Create()
    {
        GenId id = { 0, 0 };
        if (!m_freelist.TryPop(id))
        {
            id.index = (u32)m_versions.PushBack(2u);
        }
        id.version = 1u + m_versions.IncAt(id.index);
        ASSERT(id.version & 1u);
        return id;
    }

    bool Destroy(GenId id)
    {
        const u32 version = id.version;
        ASSERT(version & 1u);
        u64 spins = 0;
    tryfree:
        id.version = version;
        if (m_versions.CmpExAt(id.index, id.version, version + 1u))
        {
            id.version += 2u;
            m_freelist.Push(id);
            return true;
        }
        if (!id.version)
        {
            // locked slot
            OS::Spin(++spins * 100);
            goto tryfree;
        }
        return false;
    }

    Array<GenId> GetActive()
    {
        Array<GenId> results;
        results.Init(Alloc_Stack);
        results.Reserve(size());

        const u32* versions = m_versions.Borrow();
        const u32 len = (u32)size();
        for (u32 i = 0; i < len; ++i)
        {
            u32 v = Load(versions[i]);
            if (v & 1u)
            {
                results.PushBack({ i, v });
            }
        }
        m_versions.Return(versions);

        return results;
    }
};

template<typename T>
struct GenArray
{
    IdAllocator m_ids;
    OS::RWLock m_lock;
    T* m_ptr;
    i32 m_capacity;

    // ------------------------------------------------------------------------

    void Init(AllocType type)
    {
        m_ids.Init(type);
        m_ptr = nullptr;
    }

    void Reset()
    {
        m_ids.Reset();
        m_lock.LockWriter();
        Store(m_capacity, 0);
        Allocator::Free(m_ptr);
        m_ptr = nullptr;
        m_lock.Close();
    }

    void Clear()
    {
        m_ids.Clear();
    }

    // ------------------------------------------------------------------------

    AllocType GetAllocator() const { return m_ids.GetAllocator(); }
    i32 capacity() const { return m_ids.capacity(); }
    i32 size() const { return m_ids.size(); }

    // ------------------------------------------------------------------------

    bool IsCurrent(GenId id) const { return m_ids.IsCurrent(version, index); }

    void OnGrow(i32 newCap)
    {
        if (newCap != Load(m_capacity))
        {
            T* newPtr = Allocator::CallocT<T>(GetAllocator(), newCap);

            m_lock.LockWriter();

            T* oldPtr = LoadPtr(m_ptr);
            const i32 oldCap = Load(m_capacity);
            const bool grew = newCap > oldCap;

            if (grew)
            {
                memcpy(newPtr, oldPtr, sizeof(T) * oldCap);
                StorePtr(m_ptr, newPtr);
                Store(m_capacity, newCap);
            }

            m_lock.UnlockWriter();

            if (grew)
            {
                Allocator::Free(oldPtr);
            }
            else
            {
                Allocator::Free(newPtr);
            }
        }
    }

    GenId Create()
    {
        GenId id = m_ids.Create();
        OnGrow(m_ids.capacity());
        return id;
    }

    bool Destroy(GenId id)
    {
        return m_ids.Destroy(id);
    }

    bool Read(GenId id, T& valueOut)
    {
        OS::ReadGuard guard(m_lock);
        if (m_ids.LockSlot(id))
        {
            valueOut = m_ptr[id.index];
            m_ids.UnlockSlot(id);
            return true;
        }
        return false;
    }

    bool Write(GenId id, const T& valueIn)
    {
        OS::ReadGuard guard(m_lock);
        if (m_ids.LockSlot(id))
        {
            m_ptr[id.index] = valueIn;
            m_ids.UnlockSlot(id);
            return true;
        }
        return false;
    }

    bool Exch(GenId id, T& prevValue, T newValue)
    {
        OS::ReadGuard guard(m_lock);
        if (m_ids.LockSlot(id))
        {
            prevValue = Exchange(m_ptr[id.index], newValue);
            m_ids.UnlockSlot(id);
            return true
        }
        return false;
    }

    bool CmpEx(GenId id, T& expected, T desired)
    {
        OS::ReadGuard guard(m_lock);
        if (m_ids.LockSlot(id))
        {
            bool result = CmpExStrong(m_ptr[id.index], expected, desired);
            m_ids.UnlockSlot(id);
            return result;
        }
        return false;
    }

    Array<GenId> GetActive()
    {
        return m_ids.GetActive();
    }
};
