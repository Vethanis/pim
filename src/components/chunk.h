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
#include "containers/pipe.h"
#include "containers/generational.h"
#include "os/thread.h"
#include "os/atomics.h"
#include "threading/task_system.h"

#include "components/resources.h"

struct Entity
{
    i32 index;
    i32 version;

    bool IsNull() const { return (index | version) == 0; }
    bool IsNotNull() const { return !IsNull(); }
    u64 AsLong() const
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
        return AsLong() < rhs.AsLong();
    }
};

struct ComponentRow final : IResource
{
    TypeId m_elementType;
    u8* m_ptr;
    i32 m_capacity;

    void Init(TypeId type, i32 cap)
    {
        m_elementType = type;
        m_ptr = (u8*)Allocator::Calloc(Alloc_Pool, type.SizeOf() * cap);
        m_capacity = cap;
        InitResource(TypeOf<ComponentRow>());
    }

    void Reset()
    {
        m_lock.LockWriter();
        Allocator::Free(m_ptr);
        m_ptr = nullptr;
        ResetResource();
    }

    template<typename T>
    Slice<const T> BorrowRead()
    {
        ASSERT(TypeOf<T>() == m_elementType);
        LockReader();
        return { (const T*)m_ptr, m_capacity };
    }

    template<typename T>
    void ReturnRead(Slice<const T> loan)
    {
        ASSERT(TypeOf<T>() == m_elementType);
        ASSERT(loan.begin() == m_ptr);
        UnlockReader();
    }

    template<typename T>
    Slice<T> BorrowReadWrite()
    {
        ASSERT(TypeOf<T>() == m_elementType);
        LockWriter();
        return { (T*)m_ptr, m_capacity };
    }

    template<typename T>
    void ReturnReadWrite(Slice<T> loan)
    {
        ASSERT(TypeOf<T>() == m_elementType);
        ASSERT(loan.begin() == m_ptr);
        UnlockWriter();
    }
};

struct Chunk final : IResource
{
    Array<TypeId> m_types;
    Array<ComponentRow*> m_rows;

    ComponentRow* GetRow(TypeId type)
    {
        OS::ReadGuard guard(m_lock);
        const TypeId* types = m_types.begin();
        const i32 len = m_types.size();
        for (i32 i = 0; i < len; ++i)
        {
            if (types[i] == type)
            {
                return m_rows[i];
            }
        }
        return nullptr;
    }

    bool AddRow(TypeId type, ComponentRow* pRow)
    {
        ASSERT(pRow);
        OS::WriteGuard guard(m_lock);
    }
};
