#pragma once

#include "common/int_types.h"
#include "containers/array.h"

struct GenIdAllocator
{
    Array<i32> m_versions;
    Array<i32> m_freelist;

    inline void Init(AllocType type)
    {
        m_versions.Init(type);
        m_freelist.Init(type);
    }

    inline void Reset()
    {
        m_versions.Reset();
        m_freelist.Reset();
    }

    inline void Clear()
    {
        m_versions.Clear();
        m_freelist.Clear();
    }

    inline i32 Size() const
    {
        return m_versions.Size();
    }

    inline i32 operator[](i32 i) const
    {
        return m_versions[i];
    }

    inline bool IsCurrent(i32 version, i32 index) const
    {
        return m_versions.InRange(index) && (version == m_versions[index]);
    }

    inline void Create(i32& version, i32& index)
    {
        if (m_freelist.IsEmpty())
        {
            m_freelist.Grow() = m_versions.Size();
            m_versions.Grow() = 0;
        }
        i32 i = m_freelist.PopValue();
        i32 v = ++m_versions[i];
        version = v;
        index = i;
    }

    inline bool Destroy(i32 version, i32 index)
    {
        if (IsCurrent(version, index))
        {
            m_freelist.Grow() = index;
            ++m_versions[index];
            return true;
        }
        return false;
    }
};
