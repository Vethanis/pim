#pragma once

#include "common/guid.h"
#include "common/guid_util.h"
#include "containers/hash_set.h"
#include "common/typeid.h"
#include "os/thread.h"

struct IResource;

struct ResourceId
{
    Guid guid;
};

static constexpr Comparator<ResourceId> ResourceRefComparator = GuidBasedComparator<ResourceId>();
using ResourceIds = HashSet<ResourceId, ResourceRefComparator>;

namespace Resources
{
    bool Create(ResourceId id, IResource* pResource);
    IResource* Destroy(ResourceId id);
    IResource* Get(ResourceId id);
};

struct IResource
{
    mutable OS::RWLock m_lock;
    TypeId m_type;
    ResourceId m_id;

    void InitResource(TypeId type)
    {
        m_lock.Open();
        m_type = type;

        Guid guid;
        bool added = false;
        do
        {
            guid = CreateGuid();
            added = Resources::Create({ guid }, this);
        } while (!added);

        m_id = { guid };
    }

    void ResetResource()
    {
        Resources::Destroy(m_id);
        m_lock.Close();
    }

    ResourceId GetId() const { return m_id; }
    TypeId GetType() const { return m_type; }
    void LockReader() const { m_lock.LockReader(); }
    void UnlockReader() const { m_lock.UnlockReader(); }
    void LockWriter() { m_lock.LockWriter(); }
    void UnlockWriter() { m_lock.UnlockWriter(); }
};
