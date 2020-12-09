#pragma once

#include "common/macro.h"
#include "containers/array.hpp"
#include "containers/queue.hpp"
#include "containers/gen_id.hpp"

class IdAllocator
{
private:
    Array<u8> m_versions;
    Queue<i32> m_free;
public:

    explicit IdAllocator(EAlloc allocator = EAlloc_Perm) :
        m_versions(allocator),
        m_free(allocator)
    {}

    i32 Capacity() const { return m_versions.Size(); }
    i32 Size() const { return m_versions.Size() - m_free.Size(); }
    const Array<u8>& GetVersions() const { return m_versions; }

    void Clear()
    {
        m_versions.Clear();
        m_free.Clear();
    }

    void Reset()
    {
        m_versions.Reset();
        m_free.Reset();
    }

    bool Exists(GenId id) const
    {
        return id.version == m_versions[id.index];
    }

    GenId Alloc()
    {
        i32 index = 0;
        if (!m_free.TryPop(index))
        {
            index = m_versions.Size();
            m_versions.Grow() = 0;
        }
        u8 version = ++m_versions[index];
        GenId id;
        id.version = version;
        id.index = index;
        ASSERT(version & 1);
        return id;
    }

    bool Free(GenId id)
    {
        if (Exists(id))
        {
            i32 index = id.index;
            m_versions[index]++;
            m_free.PushCopy(index);
            ASSERT(!(m_versions[index] & 1));
            return true;
        }
        return false;
    }

    bool FreeAt(i32 index)
    {
        if (m_versions[index] & 1)
        {
            m_versions[index]++;
            m_free.PushCopy(index);
            return true;
        }
        return false;
    }

    // ------------------------------------------------------------------------

    class const_iterator
    {
    private:
        u8 const *const pim_noalias m_versions;
        const i32 m_length;
        i32 m_index;
    public:
        const_iterator(const IdAllocator& idalloc, i32 index) :
            m_versions(idalloc.m_versions.begin()),
            m_length(idalloc.m_versions.Size()),
            m_index(index)
        {
            m_index = Iterate(m_index);
        }
        bool operator!=(const const_iterator&) const { return m_index < m_length; }
        const_iterator& operator++() { m_index = Iterate(m_index + 1); return *this; }
        i32 operator*() const { return m_index; }
    private:
        i32 Iterate(i32 index) const
        {
            u8 const *const pim_noalias versions = m_versions;
            const i32 len = m_length;
            for (; index < len; ++index)
            {
                // odd versions are active
                // even are in the freelist
                if (versions[index] & 1)
                {
                    break;
                }
            }
            return index;
        }
    };
    const_iterator begin() const { return const_iterator(*this, 0); }
    const_iterator end() const { return const_iterator(*this, m_versions.Size()); }

    // ------------------------------------------------------------------------

};
