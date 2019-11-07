#pragma once

#include "common/macro.h"
#include "common/int_types.h"
#include "containers/slice.h"
#include "containers/array.h"
#include "containers/bitfield.h"
#include "components/entity.h"
#include "components/component.h"

using ComponentFlags = BitField<ComponentType_Count>;

struct EntityQuery
{
    ComponentFlags all;
    ComponentFlags any;
    ComponentFlags none;

    inline EntityQuery()
        : all(), any(), none() {}

    inline EntityQuery All(ComponentType type)
    {
        all.Set(type);
        return *this;
    }
    template<typename T>
    inline EntityQuery& All()
    {
        all.Set(Component::TypeOf<T>());
        return *this;
    }

    inline EntityQuery& Any(ComponentType type)
    {
        any.Set(type);
        return *this;
    }
    template<typename T>
    inline EntityQuery& Any()
    {
        any.Set(Component::TypeOf<T>());
        return *this;
    }
    inline EntityQuery None(ComponentType type)
    {
        none.Set(type);
        return *this;
    }
    template<typename T>
    inline EntityQuery& None()
    {
        none.Set(Component::TypeOf<T>());
        return *this;
    }
};

namespace Ecs
{
    void Init();
    void Shutdown();

    Slice<const Entity> Search(EntityQuery query, Array<Entity>& results);

    // ------------------------------------------------------------------------

    struct Row
    {
        Array<u16> m_versions;
        Array<u8> m_components;
        ComponentType m_type;
        i32 m_count;

        void Reset();

        inline bool IsEmpty() const
        {
            return m_count == 0;
        }

        inline bool IsNull() const
        {
            return m_type == ComponentType_Null;
        }

        template<typename T>
        inline bool SameType() const
        {
            return Component::TypeOf<T>() == m_type;
        }

        inline Slice<const u16> Versions() const
        {
            return m_versions;
        }

        inline Slice<const u8> Components() const
        {
            return m_components;
        }
        inline Slice<u8> Components()
        {
            return m_components;
        }

        template<typename T>
        inline Slice<const T> Components() const
        {
            DebugAssert((m_versions.empty() && IsNull()) || SameType<T>());
            return m_components.cast<T>();
        }
        template<typename T>
        inline Slice<T> Components()
        {
            DebugAssert((m_versions.empty() && IsNull()) || SameType<T>());
            return m_components.cast<T>();
        }

        inline bool Has(Entity entity) const
        {
            DebugAssert(entity.IsNotNull());
            return m_versions.in_range(entity.index) &&
                (m_versions[entity.index] == entity.version);
        }

        inline bool Add(Entity entity, ComponentType type)
        {
            DebugAssert(entity.IsNotNull());
            const i32 i = entity.index;

            bool inRange = m_versions.in_range(i);
            const u16 curVersion = inRange ? m_versions[i] : 0;
            if (curVersion)
            {
                return curVersion == entity.version;
            }

            if (IsNull())
            {
                m_type = type;
            }
            DebugAssert(m_type == type);

            const i32 sizeOf = Component::SizeOf(type);

            if (!inRange)
            {
                const i32 prevLen = m_versions.size();
                const i32 newLen = i + 1;
                m_versions.resize(i + 1);
                for (i32 iVersion = prevLen; iVersion < newLen; ++iVersion)
                {
                    m_versions[iVersion] = 0;
                }
                m_components.resize(newLen * sizeOf);
            }

            m_versions[i] = entity.version;
            memset(m_components.at(i * sizeOf), 0, sizeOf);
            ++m_count;

            return true;
        }

        template<typename T>
        inline bool Add(Entity entity)
        {
            return Add(entity, Component::TypeOf<T>());
        }

        inline bool Remove(Entity entity)
        {
            if (Has(entity))
            {
                Component::Drop(m_type, Get(entity));
                m_versions[entity.index] = 0;
                --m_count;
                return true;
            }
            return false;
        }

        inline void* Get(Entity entity)
        {
            DebugAssert(Has(entity));
            return m_components.at(entity.index * Component::SizeOf(m_type));
        }

        template<typename T>
        inline T& Get(Entity entity)
        {
            DebugAssert(SameType<T>());
            DebugAssert(Has(entity));
            return *(m_components.as<T>(entity.index));
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

        inline Row& GetRow(ComponentType type)
        {
            DebugAssert(Component::ValidType(type));
            return m_rows[type];
        }
        inline const Row& GetRow(ComponentType type) const
        {
            DebugAssert(Component::ValidType(type));
            return m_rows[type];
        }
        template<typename T>
        inline Row& GetRow()
        {
            return GetRow(Component::TypeOf<T>());
        }
        template<typename T>
        inline const Row& GetRow() const
        {
            return GetRow(Component::TypeOf<T>());
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

        inline Slice<u8> Components(ComponentType type)
        {
            return GetRow(type).Components();
        }
        template<typename T>
        inline Slice<T> Components()
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

        inline void Clear(ComponentType type)
        {
            if (m_rowFlags.Has(type))
            {
                m_rowFlags.UnSet(type);
                GetRow(type).Reset();
            }
        }

        template<typename T>
        inline void Clear()
        {
            Clear(Component::TypeOf<T>());
        }

        inline bool Has(Entity entity, ComponentType type) const
        {
            DebugAssert(entity.table == m_id);
            DebugAssert(InRange(entity));
            DebugAssert(IsCurrent(entity));
            return GetRow(type).Has(entity);
        }

        template<typename T>
        inline bool Has(Entity entity) const
        {
            return Has(entity, Component::TypeOf<T>());
        }

        inline bool Add(Entity entity, ComponentType type)
        {
            DebugAssert(entity.table == m_id);
            DebugAssert(InRange(entity));
            DebugAssert(IsCurrent(entity));
            if (GetRow(type).Add(entity, type))
            {
                m_rowFlags.Set(type);
                m_entityFlags[entity.index].Set(type);
                return true;
            }
            return false;
        }

        template<typename T>
        inline bool Add(Entity entity)
        {
            return Add(entity, Component::TypeOf<T>());
        }

        inline bool Remove(Entity entity, ComponentType type)
        {
            DebugAssert(entity.table == m_id);
            Row& row = GetRow(type);
            if (row.Remove(entity))
            {
                m_entityFlags[entity.index].UnSet(type);
                if (row.IsEmpty())
                {
                    row.Reset();
                    m_rowFlags.UnSet(type);
                }
                return true;
            }
            return false;
        }

        template<typename T>
        inline bool Remove(Entity entity)
        {
            return Remove(entity, Component::TypeOf<T>());
        }

        inline void* Get(Entity entity, ComponentType type)
        {
            DebugAssert(entity.table == m_id);
            DebugAssert(InRange(entity));
            DebugAssert(IsCurrent(entity));
            return GetRow(type).Get(entity);
        }

        template<typename T>
        inline T& Get(Entity entity)
        {
            DebugAssert(entity.table == m_id);
            DebugAssert(InRange(entity));
            DebugAssert(IsCurrent(entity));
            return GetRow<T>().Get<T>(entity);
        }
    };

    // ------------------------------------------------------------------------

    Slice<Table> Tables();

    inline Table& GetTable(TableId tableId)
    {
        return Tables()[tableId];
    }

    inline Table& GetTable(Entity entity)
    {
        return Tables()[entity.table];
    }

    inline Entity Create(TableId tableId)
    {
        return GetTable(tableId).Create();
    }

    inline bool Destroy(Entity entity)
    {
        return GetTable(entity).Destroy(entity);
    }

    inline bool Has(Entity entity, ComponentType type)
    {
        return GetTable(entity).Has(entity, type);
    }

    template<typename T>
    inline bool Has(Entity entity)
    {
        return Has(entity, Component::TypeOf<T>());
    }

    inline bool Add(Entity entity, ComponentType type)
    {
        return GetTable(entity).Add(entity, type);
    }

    template<typename T>
    inline bool Add(Entity entity)
    {
        return Add(entity, Component::TypeOf<T>());
    }

    inline bool Remove(Entity entity, ComponentType type)
    {
        return GetTable(entity).Remove(entity, type);
    }

    template<typename T>
    inline bool Remove(Entity entity)
    {
        return Remove(entity, Component::TypeOf<T>());
    }

    inline void* Get(Entity entity, ComponentType type)
    {
        return GetTable(entity).Get(entity, type);
    }

    template<typename T>
    inline T& Get(Entity entity)
    {
        return GetTable(entity).Get<T>(entity);
    }
};
