#pragma once

#include "common/macro.h"
#include "common/int_types.h"
#include "common/typeinfo.h"
#include "containers/slice.h"
#include "containers/array.h"
#include "containers/bitfield.h"
#include "components/entity.h"

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
    inline bool Has(Entity entity) const
    {
        return InRange(entity.index) && (m_compVersions[entity.index] == entity.version);
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
    inline T* Get(Entity entity)
    {
        return Has(entity) ? m_components + entity.index : nullptr;
    }
    inline const T* Get(Entity entity) const
    {
        return Has(entity) ? m_components + entity.index : nullptr;
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
    inline T& operator[](Entity entity)
    {
        DebugAssert(Has(entity));
        return m_components[entity.index];
    }
    inline const T& operator[](Entity entity) const
    {
        DebugAssert(Has(entity));
        return m_components[entity.index];
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

using ComponentFlags = BitField<ComponentType_Count>;

struct EntityQuery
{
    ComponentFlags all;
    ComponentFlags any;
    ComponentFlags none;

    inline EntityQuery()
        : all(), any(), none() {}

    template<typename T>
    inline EntityQuery& All()
    {
        all.Set(T::C_Type);
        return *this;
    }
    template<typename T>
    inline EntityQuery& Any()
    {
        any.Set(T::C_Type);
        return *this;
    }
    template<typename T>
    inline EntityQuery& None()
    {
        none.Set(T::C_Type);
        return *this;
    }
};

namespace Ecs
{
    void Init();
    void Shutdown();

    Slice<const Entity> ForEach(EntityQuery query, Array<Entity>& results);

    // ------------------------------------------------------------------------

    struct Row
    {
        using DestroyCb = void(*)(void*);

        Array<u16> m_versions;
        Array<u8> m_components;
        u32 m_stride;
        i32 m_count;
        DestroyCb m_onDestroy;

        void Reset();

        inline bool IsEmpty() const
        {
            return m_count == 0;
        }

        inline bool IsNull() const
        {
            return m_stride == 0;
        }

        inline Slice<const u16> Versions() const
        {
            return m_versions;
        }

        template<typename T>
        inline CompArray<T> Components()
        {
            DebugAssert(
                (m_versions.empty() && IsNull()) ||
                (m_stride == sizeof(T)));
            return CompArray<T>(m_versions, m_components.cast<T>());
        }

        inline bool Has(Entity entity) const
        {
            DebugAssert(entity.IsNotNull());

            return m_versions.in_range(entity.index) &&
                (m_versions[entity.index] == entity.version);
        }

        template<typename T>
        inline T& Add(Entity entity)
        {
            DebugAssert(entity.IsNotNull());

            if (m_stride == 0)
            {
                m_type = T::C_Type;
                m_stride = sizeof(T);
                m_onDestroy = (DestroyCb)T::OnDestroy;
            }
            DebugAssert(m_stride == sizeof(T));

            const i32 i = entity.index;
            if (!m_versions.in_range(i))
            {
                m_versions.resize(i + 1);
                m_components.resize((i + 1) * sizeof(T));
                m_components.embed<T>();
            }

            if (m_versions[i] == 0)
            {
                m_versions[i] = entity.version;
                ++m_count;
            }

            return m_components.as<T>(entity.index);
        }

        inline bool Remove(Entity entity)
        {
            if (Has(entity))
            {
                DebugAssert(m_stride != 0);
                DebugAssert(m_onDestroy);

                void* pComponent = m_components.at(entity.index * m_stride);
                m_onDestroy(pComponent);
                m_versions[entity.index] = 0;
                --m_count;

                return true;
            }
            return false;
        }

        template<typename T>
        inline T& Get(Entity entity)
        {
            DebugAssert(entity.IsNotNull());
            DebugAssert(Has(entity));
            DebugAssert(m_stride == sizeof(T));

            return *m_components.as<T>(entity.index);
        }
    };

    // ------------------------------------------------------------------------

    struct Table
    {
        TableId m_id;
        ComponentFlags m_rowFlags;
        Array<Entity> m_entities;
        Array<ComponentFlags> m_entityFlags;
        Array<u16> m_versions;
        Array<u16> m_freelist;
        Row m_rows[ComponentType_Count];

        void Reset();
        Entity Create();
        bool Destroy(Entity entity);

        template<typename T>
        inline Row& GetRow()
        {
            return m_rows[T::C_Type];
        }

        inline i32 Size() const
        {
            return m_versions.size();
        }

        inline bool InRange(Entity entity) const
        {
            DebugAssert(entity.table == m_id);
            return m_versions.in_range(entity.index);
        }

        inline bool IsCurrent(Entity entity) const
        {
            DebugAssert(entity.table == m_id);
            return m_versions[entity.index] == entity.version;
        }

        inline bool IsCurrent(i32 i) const
        {
            return m_versions[i] == m_entities[i].version;
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
        inline CompArray<T> Components()
        {
            return GetRow<T>().Components<T>();
        }

        inline ComponentFlags RowFlags() const
        {
            return m_rowFlags;
        }

        inline Slice<const ComponentFlags> EntityFlags() const
        {
            return m_entityFlags;
        }

        template<typename T>
        inline void Clear()
        {
            if (m_rowFlags.Has(T::C_Type))
            {
                m_rowFlags.UnSet(T::C_Type);
                GetRow<T>().Reset();
            }
        }

        template<typename T>
        inline bool HasComponent(Entity entity)
        {
            DebugAssert(entity.table == m_id);
            DebugAssert(InRange(entity));
            DebugAssert(IsCurrent(entity));
            return GetRow<T>().Has(entity);
        }

        template<typename T>
        inline T& AddComponent(Entity entity)
        {
            DebugAssert(entity.table == m_id);
            DebugAssert(InRange(entity));
            DebugAssert(IsCurrent(entity));
            m_rowFlags.Set(T::C_Type);
            m_entityFlags[entity.index].Set(T::C_Type);
            return GetRow<T>().Add<T>(entity);
        }

        template<typename T>
        inline bool RemoveComponent(Entity entity)
        {
            DebugAssert(entity.table == m_id);
            Row& row = GetRow<T>();
            if (row.Remove(entity))
            {
                m_entityFlags[entity.index].UnSet(T::C_Type);
                if (row.IsEmpty())
                {
                    row.Reset();
                }
                return true;
            }
            return false;
        }

        template<typename T>
        inline T& GetComponent(Entity entity)
        {
            DebugAssert(entity.table == m_id);
            DebugAssert(InRange(entity));
            DebugAssert(IsCurrent(entity));
            return GetRow<T>().Get<T>(entity);
        }
    };

    // ------------------------------------------------------------------------

    Table& GetTable(TableId chunkId);
    Table& GetTable(Entity entity);
    Slice<Table> Tables();

    template<typename T>
    inline bool HasComponent(Entity entity)
    {
        return GetTable(entity).HasComponent<T>(entity);
    }

    template<typename T>
    inline T& AddComponent(Entity entity)
    {
        return GetTable(entity).AddComponent<T>(entity);
    }

    template<typename T>
    inline bool RemoveComponent(Entity entity)
    {
        return GetTable(entity).RemoveComponent<T>(entity);
    }

    template<typename T>
    inline T& GetComponent(Entity entity)
    {
        return GetTable(entity).GetComponent<T>(entity);
    }
};
