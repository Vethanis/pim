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
    struct CompArray
    {
        const i32 m_size;
        const u16* const m_compVersions;
        T* const m_components;

        inline CompArray(
            Slice<const u16> compVersions,
            Slice<T> components)
            : m_size(compVersions.size()),
            m_compVersions(compVersions.begin()),
            m_components(components.begin())
        {}

        inline i32 Size() const
        {
            return m_size;
        }
        inline bool InRange(i32 i) const
        {
            return (u32)i < (u32)m_size;
        }
        inline bool Has(i32 i) const
        {
            return m_compVersions[i] != 0;
        }
        inline i32 Iterate(i32 i) const
        {
            for (; InRange(i); ++i)
            {
                if (Has(i))
                {
                    break;
                }
            }
            return i;
        }
        inline T* Get(i32 i)
        {
            DebugAssert(InRange(i));
            return Has(i) ? m_components + i : nullptr;
        }
        inline const T* Get(i32 i) const
        {
            DebugAssert(InRange(i));
            return Has(i) ? m_components + i : nullptr;
        }
        inline T& operator[](i32 i)
        {
            DebugAssert(InRange(i));
            DebugAssert(Has(i));
            return m_components[i];
        }
        inline const T& operator[](i32 i) const
        {
            DebugAssert(InRange(i));
            DebugAssert(Has(i));
            return m_components[i];
        }

        struct iterator
        {
            CompArray m_array;
            u32 m_i;

            inline iterator(CompArray src, i32 i)
                : m_array(src)
            {
                m_i = m_array.Iterate(i);
            }
            inline bool operator !=(const iterator&) const
            {
                return m_array.InRange(m_i);
            }
            inline iterator& operator++()
            {
                m_i = m_array.Iterate(m_i + 1);
                return *this;
            }
            inline T& operator *()
            {
                return m_array[m_i];
            }
        }; // iterator

        inline iterator begin()
        {
            return iterator(*this, 0);
        }
        inline iterator end()
        {
            return iterator(*this, Size());
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

        inline bool IsNull() const
        {
            return m_type == ComponentType_Null;
        }

        inline i32 Size() const
        {
            return m_versions.size();
        }

        inline bool InRange(i32 i) const
        {
            return m_versions.in_range(i);
        }

        inline bool InRange(Entity entity) const
        {
            return InRange(entity.index);
        }

        inline Slice<const u16> Versions() const
        {
            return m_versions;
        }

        template<typename T>
        inline CompArray<T> Components()
        {
            DebugAssert(IsNull() || T::C_Type == m_type);
            return CompArray<T>(m_versions, m_components.cast<T>());
        }

        inline bool Has(Entity entity) const
        {
            return m_versions.in_range(entity.index) &&
                (m_versions[entity.index] == entity.version);
        }

        template<typename T>
        inline T& Add(Entity entity)
        {
            if (IsNull())
            {
                Setup<T>();
            }
            DebugAssert(T::C_Type == m_type);
            if (!m_versions.in_range(entity.index))
            {
                m_versions.resize(entity.index + 1);
                m_components.resize((entity.index + 1) * sizeof(T));
            }
            m_versions[entity.index] = entity.version;
            m_components.embed<T>();
        }

        template<typename T>
        inline void Add(Entity entity, T component)
        {
            Add<T>(entity) = component;
        }

        inline bool Remove(Entity entity)
        {
            if (Has(entity))
            {
                DebugAssert(m_stride != 0);
                m_onDestroy(m_components.at(entity.index * m_stride));
                m_versions[entity.index] = 0;
                return true;
            }
            return false;
        }

        template<typename T>
        inline T& Get(Entity entity)
        {
            DebugAssert(Has(entity));
            DebugAssert(T::C_Type == m_type);
            return *m_components.as<T>(entity.index);
        }

        template<typename T>
        inline void Set(Entity entity, T component)
        {
            Get(entity) = component;
        }
    };

    // ------------------------------------------------------------------------

    struct Table
    {
        Array<Entity> m_entities;
        Array<u16> m_versions;
        Array<u16> m_freelist;
        Row m_rows[ComponentType_Count];

        void Reset();
        Entity Create(TableId chunkId);
        bool Destroy(Entity entity);

        inline i32 Size() const
        {
            return m_versions.size();
        }

        inline bool InRange(i32 i) const
        {
            return m_versions.in_range(i);
        }

        inline bool InRange(Entity entity) const
        {
            return m_versions.in_range(entity.index);
        }

        inline bool IsCurrent(Entity entity) const
        {
            return m_versions[entity.index] == entity.version;
        }

        inline Slice<const Entity> Entities() const
        {
            return m_entities;
        }

        inline Slice<const u16> Versions() const
        {
            return m_versions;
        }

        template<typename T>
        inline Row& GetRow()
        {
            return m_rows[T::C_Type];
        }

        template<typename T>
        inline CompArray<T> Components()
        {
            return GetRow<T>().Components<T>();
        }

        template<typename T>
        inline bool HasRow() const
        {
            return !GetRow<T>().IsNull();
        }

        template<typename T>
        inline void RemoveRow()
        {
            GetRow<T>().Reset();
        }

        template<typename T>
        inline bool HasComponent(Entity entity)
        {
            DebugAssert(InRange(entity));
            DebugAssert(IsCurrent(entity));
            return GetRow<T>().Has(entity);
        }

        template<typename T>
        inline T& AddComponent(Entity entity)
        {
            DebugAssert(InRange(entity));
            DebugAssert(IsCurrent(entity));
            return GetRow<T>().Add<T>(entity);
        }

        template<typename T>
        inline void AddComponent(Entity entity, T component)
        {
            AddComponent<T>(entity) = component;
        }

        template<typename T>
        inline bool RemoveComponent(Entity entity)
        {
            DebugAssert(InRange(entity));
            DebugAssert(IsCurrent(entity));
            return GetRow<T>().Remove(entity);
        }

        template<typename T>
        inline T& GetComponent(Entity entity)
        {
            DebugAssert(InRange(entity));
            DebugAssert(IsCurrent(entity));
            return GetRow<T>().Get<T>(entity);
        }

        template<typename T>
        inline void SetComponent(Entity entity, T component)
        {
            DebugAssert(InRange(entity));
            DebugAssert(IsCurrent(entity));
            GetRow<T>.Set<T>(entity, component);
        }
    };

    // ------------------------------------------------------------------------
};
