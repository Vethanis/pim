#pragma once

#include "common/int_types.h"
#include "containers/array.h"

struct IdAllocator
{
    Array<u16> m_versions;
    Array<u16> m_freelist;

    // ------------------------------------------------------------------------

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

    inline void Trim()
    {
        m_versions.Trim();
        m_freelist.Trim();
    }

    // ------------------------------------------------------------------------

    inline i32 Size() const
    {
        return m_versions.Size();
    }

    inline u16 operator[](i32 i) const
    {
        return m_versions[i];
    }

    inline bool IsCurrent(u16 version, u16 index) const
    {
        return version == m_versions[index];
    }

    // ------------------------------------------------------------------------

    inline bool Create(u16& version, u16& index)
    {
        bool grew = false;
        if (m_freelist.IsEmpty())
        {
            m_freelist.Grow() = m_versions.Size();
            m_versions.Grow() = 0;
            grew = true;
        }
        u16 i = m_freelist.PopValue();
        u16 v = ++m_versions[i];
        ASSERT(v & 1);
        version = v;
        index = i;
        return grew;
    }

    inline bool Destroy(u16 version, u16 index)
    {
        if (IsCurrent(version, index))
        {
            m_freelist.Grow() = index;
            u16 v = ++m_versions[index];
            ASSERT(!(v & 1));
            return true;
        }
        return false;
    }
};

// ------------------------------------------------------------------------

struct IdMapping
{
    IdAllocator m_allocator;    // global ids
    Array<u16> m_toLocal;       // parallel to global ids

    Array<u16> m_toGlobal;      // parallel to local storage

    // ------------------------------------------------------------------------

    inline void Init(AllocType type)
    {
        m_allocator.Init(type);
        m_toLocal.Init(type);
        m_toGlobal.Init(type);
    }

    inline void Reset()
    {
        m_allocator.Reset();
        m_toLocal.Reset();
        m_toGlobal.Reset();
    }

    inline void Clear()
    {
        m_allocator.Clear();
        m_toLocal.Clear();
        m_toGlobal.Clear();
    }

    inline void Trim()
    {
        m_allocator.Trim();
        m_toLocal.Trim();
        m_toGlobal.Trim();
    }

    // ------------------------------------------------------------------------

    inline bool IsCurrent(u16 version, u16 globalIndex) const
    {
        return m_allocator.IsCurrent(version, globalIndex);
    }

    // ------------------------------------------------------------------------

    inline i32 LocalSize() const
    {
        return m_toGlobal.Size();
    }

    inline i32 LocalCapacity() const
    {
        return m_toGlobal.Capacity();
    }

    inline u16 ToGlobal(u16 localIndex) const
    {
        return m_toGlobal[localIndex];
    }

    inline Slice<const u16> ToGlobals() const
    {
        return m_toGlobal;
    }

    // ------------------------------------------------------------------------

    inline i32 GlobalSize() const
    {
        return m_toLocal.Size();
    }

    inline i32 GlobalCapacity() const
    {
        return m_toLocal.Capacity();
    }

    inline u16 ToLocal(u16 globalIndex) const
    {
        return m_toLocal[globalIndex];
    }

    inline Slice<const u16> ToLocals() const
    {
        return m_toLocal;
    }

    // ------------------------------------------------------------------------

    inline bool Create(u16& version, u16& index)
    {
        bool grew = false;
        if (m_allocator.Create(version, index))
        {
            m_toLocal.Grow() = 0xffff;
            grew = true;
        }
        m_toLocal[index] = u16(m_toGlobal.Size());
        m_toGlobal.Grow() = index;
        return grew;
    }

    inline bool Destroy(u16 version, u16 index)
    {
        if (m_allocator.Destroy(version, index))
        {
            // x[i] = x[--length]
            // make globalB point to localA
            // and localA point to globalB

            const u16 globalA = index;
            const u16 localA = m_toLocal[globalA];
            const u16 localB = u16(m_toGlobal.Size() - 1);
            const u16 globalB = m_toGlobal[localB];

            m_toLocal[globalB] = localA;
            m_toLocal[globalA] = 0xffff;

            m_toGlobal[localA] = globalB;
            m_toGlobal[localB] = 0xffff;

            m_toGlobal.Pop();

            return true;
        }
        return false;
    }
};
