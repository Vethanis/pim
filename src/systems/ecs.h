#pragma once

#include "common/macro.h"
#include "common/int_types.h"
#include "common/typeinfo.h"
#include "containers/slice.h"
#include "containers/array.h"
#include "containers/dict.h"
#include "components/entity.h"

namespace Ecs
{
    void Init();
    void Shutdown();

    // ------------------------------------------------------------------------

    struct Table;

    Table& GetTable(TableId chunkId);
    Table& GetTable(Entity entity);
    Slice<Table> Tables();

    // ------------------------------------------------------------------------

    template<typename T>
    struct Comps
    {
        Slice<const u16> entVersions;
        Slice<const u16> compVersions;
        Slice<T> components;

        inline bool Has(i32 i) const
        {
            return entVersions[i] == compVersions[i];
        }
        inline i32 Size() const
        {
            return compVersions.size();
        }
        inline T* Get(i32 i)
        {
            return Has(i) ? &(components[i]) : nullptr;
        }
        inline const T* Get(i32 i) const
        {
            return Has(i) ? &(components[i]) : nullptr;
        }
        inline T& operator[](i32 i)
        {
            DebugAssert(Has(i));
            return components[i];
        }
        inline const T& operator[](i32 i) const
        {
            DebugAssert(Has(i));
            return components[i];
        }

        struct iterator
        {
            u32 m_i;
            const u32 m_size;
            const u16* m_entVersions;
            const u16* m_compVersions;
            T* m_components;

            inline iterator(Comps& comps, i32 i) : m_size(comps.entVersions.size())
            {
                m_i = i;
                m_entVersions = comps.entVersions.begin();
                m_compVersions = comps.compVersions.begin();
                m_components = comps.components.begin();
            }

            inline bool operator !=(const iterator&) const
            {
                return (m_i < m_size) && (m_entVersions[m_i] == m_compVersions[m_i]);
            }
            inline iterator& operator++()
            {
                const i32 count = m_size;
                auto ents = m_entVersions;
                auto comps = m_compVersions;
                i32 i = m_i + 1;
                for (; i < count; ++i)
                {
                    if (ents[i] == comps[i])
                    {
                        break;
                    }
                }
                m_i = i;
                return *this;
            }
            inline T& operator *()
            {
                return m_components[m_i];
            }
        }; // iterator

        inline iterator begin()
        {
            return iterator(*this, 0);
        }
        inline iterator end()
        {
            return iterator(*this, components.size());
        }
    };

    struct Row
    {
        using DestroyCb = void(*)(void*);

        Array<u16> m_versions;
        Array<u8> m_components;
        ComponentType m_type;
        u32 m_stride;
        DestroyCb m_onDestroy;

        void Reset();

        template<typename T>
        inline void Setup()
        {
            m_type = T::C_Type;
            m_stride = sizeof(T);
            m_onDestroy = (DestroyCb)T::OnDestroy;
        }

        inline Slice<const u16> Versions() const
        {
            return m_versions;
        }

        template<typename T>
        inline Slice<const T> Components() const
        {
            DebugAssert(T::C_Type == m_type);
            return m_components.cast<const T>();
        }

        template<typename T>
        inline Slice<T> Components()
        {
            DebugAssert(T::C_Type == m_type);
            return m_components.cast<T>();
        }

        inline bool Has(Entity entity) const
        {
            return m_versions.in_range(entity.index) &&
                (m_versions[entity.index] == entity.version);
        }

        template<typename T>
        inline void Add(Entity entity)
        {
            DebugAssert(entity.IsNotNull());
            DebugAssert(T::C_Type == m_type);
            if (!m_versions.in_range(entity.index))
            {
                m_versions.resize(entity.index + 1);
                m_components.resize((entity.index + 1) * sizeof(T));
            }
            m_versions[entity.index] = entity.version;
            m_components.embed<T>();
        }

        inline bool Remove(Entity entity)
        {
            DebugAssert(m_stride != 0);
            if (Has(entity))
            {
                m_onDestroy(m_components.at(entity.index * m_stride));
                m_versions[entity.index] = 0;
                return true;
            }
            return false;
        }

        template<typename T>
        inline T* Get(Entity entity)
        {
            if (Has(entity))
            {
                DebugAssert(T::C_Type == m_type);
                return m_components.as<T>(entity.index);
            }
            return nullptr;
        }
    };

    // ------------------------------------------------------------------------

    struct Table
    {
        Array<u16> m_versions;
        Array<u16> m_freelist;
        Row m_rows[ComponentType_Count];
        bool m_has[ComponentType_Count];

        void Reset();

        inline bool IsCurrent(Entity entity) const
        {
            return m_versions.in_range(entity.index) &&
                (m_versions[entity.index] == entity.version);
        }

        Entity Create(TableId chunkId);
        bool Destroy(Entity entity);

        inline Slice<const u16> Versions() const
        {
            return m_versions;
        }

        template<typename T>
        inline bool HasRow() const
        {
            return m_has[T::C_Type];
        }

        template<typename T>
        inline void AddRow()
        {
            i32 i = T::C_Type;
            if (!m_has[i])
            {
                m_has[i] = true;
                m_rows[T::C_Type].Setup<T>();
            }
        }

        template<typename T>
        inline void RemoveRow()
        {
            i32 i = T::C_Type;
            if (m_has[i])
            {
                m_has[i] = false;
                m_rows[i].Reset();
            }
        }

        template<typename T>
        inline Row& GetRow()
        {
            i32 i = T::C_Type;
            DebugAssert(m_has[i]);
            return ;
        }

        template<typename T>
        inline Comps<T> Get()
        {
            Row& row = m_rows[T::C_Type];
            Comps<T> slice = {};
            slice.entVersions = m_versions;
            slice.compVersions = row.m_versions;
            slice.components = row.m_components.cast<T>();
            return slice;
        }
    };

    // ------------------------------------------------------------------------
};
