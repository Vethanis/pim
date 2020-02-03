#pragma once

#include "common/typeid.h"
#include "os/thread.h"
#include "os/atomics.h"

struct IResource
{
    OS::RWLock m_lock;
    TypeId m_type;
    i32 m_refCount;

    void LockReader() const { m_lock.LockReader(); }
    void UnlockReader() const { m_lock.UnlockReader(); }
    void LockWriter() { m_lock.LockWriter(); }
    void UnlockWriter() { m_lock.UnlockWriter(); }
    TypeId GetType() const { return m_type; }
    i32 IncRefs() { return 1 + Inc(m_refCount); }
    i32 DecRefs() { return Dec(m_refCount) - 1; }
};

struct ResourceRef
{
    IResource* pResource;
    bool readOnly;

    void Lock()
    {
        ASSERT(pResource);
        if (readOnly)
            pResource->LockReader();
        else
            pResource->LockWriter();
    }

    void Unlock()
    {
        ASSERT(pResource);
        if (readOnly)
            pResource->UnlockReader();
        else
            pResource->UnlockWriter();
    }

    static bool Equals(const ResourceRef& lhs, const ResourceRef& rhs)
    {
        return lhs.pResource == rhs.pResource;
    }
    static i32 Compare(const ResourceRef& lhs, const ResourceRef& rhs)
    {
        return (i32)(lhs.pResource - rhs.pResource);
    }
    static u32 Hash(const ResourceRef& x)
    {
        return Fnv32Qword((u64)x.pResource);
    }

    static constexpr Comparator<ResourceRef> Cmp = { Equals, Compare, Hash };
};

namespace Resources
{

};
