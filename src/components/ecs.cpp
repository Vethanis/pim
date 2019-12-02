#include "components/ecs.h"
#include "common/stringutil.h"

namespace Ecs
{
    static Table ms_tables[TableId_Count];
    static constexpr Slice<Table> ms_slice = { ms_tables, TableId_Count };

    void Init()
    {
        MemClear(ms_tables, sizeof(ms_tables));
        for (i32 i = 0; i < TableId_Count; ++i)
        {
            ms_tables[i].m_id = (TableId)i;
        }
    }

    void Shutdown()
    {
        for (Table& chunk : ms_tables)
        {
            chunk.Reset();
        }
    }

    Slice<Table> Tables()
    {
        return ms_slice;
    }

    static bool HasAll(ComponentFlags rowFlags, ComponentFlags flags)
    {
        return (rowFlags & flags) == flags;
    }

    static bool HasAny(ComponentFlags rowFlags, ComponentFlags flags)
    {
        return (rowFlags & flags).Any();
    }

    static bool HasNone(ComponentFlags rowFlags, ComponentFlags flags)
    {
        return !HasAny(rowFlags, flags);
    }

    static void SearchAll(ComponentFlags all, Array<Entity>& results)
    {
        for (const Table& table : ms_tables)
        {
            if (HasAll(table.RowFlags(), all))
            {
                auto entities = table.Entities();
                auto versions = table.Versions();
                auto entityFlags = table.EntityFlags();
                const i32 count = entities.Size();
                for (i32 i = 0; i < count; ++i)
                {
                    if ((entities[i].version == versions[i]) &&
                        HasAll(entityFlags[i], all))
                    {
                        results.Grow() = entities[i];
                    }
                }
            }
        }
    }

    static void SearchAny(ComponentFlags any, Array<Entity>& results)
    {
        for (const Table& table : ms_tables)
        {
            if (HasAny(table.RowFlags(), any))
            {
                auto entities = table.Entities();
                auto versions = table.Versions();
                auto entityFlags = table.EntityFlags();
                const i32 count = entities.Size();
                for (i32 i = 0; i < count; ++i)
                {
                    if ((entities[i].version == versions[i]) &&
                        HasAny(entityFlags[i], any))
                    {
                        results.Grow() = entities[i];
                    }
                }
            }
        }
    }

    static void SearchNone(ComponentFlags none, Array<Entity>& results)
    {
        for (const Table& table : ms_tables)
        {
            if (HasNone(table.RowFlags(), none))
            {
                auto entities = table.Entities();
                auto versions = table.Versions();
                auto entityFlags = table.EntityFlags();
                const i32 count = entities.Size();
                for (i32 i = 0; i < count; ++i)
                {
                    if ((entities[i].version == versions[i]) &&
                        HasNone(entityFlags[i], none))
                    {
                        results.Grow() = entities[i];
                    }
                }
            }
        }
    }

    static void SearchAllAny(
        ComponentFlags all,
        ComponentFlags any,
        Array<Entity>& results)
    {
        for (const Table& table : ms_tables)
        {
            auto rowFlags = table.RowFlags();
            if (HasAll(rowFlags, all) &&
                HasAny(rowFlags, any))
            {
                auto entities = table.Entities();
                auto versions = table.Versions();
                auto entityFlags = table.EntityFlags();
                const i32 count = entities.Size();
                for (i32 i = 0; i < count; ++i)
                {
                    if ((entities[i].version == versions[i]) &&
                        HasAll(entityFlags[i], all) &&
                        HasAny(entityFlags[i], any))
                    {
                        results.Grow() = entities[i];
                    }
                }
            }
        }
    }

    static void SearchAllNone(
        ComponentFlags all,
        ComponentFlags none,
        Array<Entity>& results)
    {
        for (const Table& table : ms_tables)
        {
            auto rowFlags = table.RowFlags();
            if (HasAll(rowFlags, all) &&
                HasNone(rowFlags, none))
            {
                auto entities = table.Entities();
                auto versions = table.Versions();
                auto entityFlags = table.EntityFlags();
                const i32 count = entities.Size();
                for (i32 i = 0; i < count; ++i)
                {
                    if ((entities[i].version == versions[i]) &&
                        HasAll(entityFlags[i], all) &&
                        HasNone(entityFlags[i], none))
                    {
                        results.Grow() = entities[i];
                    }
                }
            }
        }
    }

    static void SearchAnyNone(
        ComponentFlags any,
        ComponentFlags none,
        Array<Entity>& results)
    {
        for (const Table& table : ms_tables)
        {
            auto rowFlags = table.RowFlags();
            if (HasAny(rowFlags, any) &&
                HasNone(rowFlags, none))
            {
                auto entities = table.Entities();
                auto versions = table.Versions();
                auto entityFlags = table.EntityFlags();
                const i32 count = entities.Size();
                for (i32 i = 0; i < count; ++i)
                {
                    if ((entities[i].version == versions[i]) &&
                        HasAny(entityFlags[i], any) &&
                        HasNone(entityFlags[i], none))
                    {
                        results.Grow() = entities[i];
                    }
                }
            }
        }
    }

    static void SearchAllAnyNone(
        ComponentFlags all,
        ComponentFlags any,
        ComponentFlags none,
        Array<Entity>& results)
    {
        for (const Table& table : ms_tables)
        {
            auto rowFlags = table.RowFlags();
            if (HasAll(rowFlags, all) &&
                HasAny(rowFlags, any) &&
                HasNone(rowFlags, none))
            {
                auto entities = table.Entities();
                auto versions = table.Versions();
                auto entityFlags = table.EntityFlags();
                const i32 count = entities.Size();
                for (i32 i = 0; i < count; ++i)
                {
                    if ((entities[i].version == versions[i]) &&
                        HasAll(entityFlags[i], all) &&
                        HasAny(entityFlags[i], any) &&
                        HasNone(entityFlags[i], none))
                    {
                        results.Grow() = entities[i];
                    }
                }
            }
        }
    }

    static Array<Entity> ms_results;

    Slice<const Entity> Search(EntityQuery query)
    {
        ms_results.Clear();

        u32 bits = 0;
        bits |= query.all.Any() ? 1 : 0;
        bits |= query.any.Any() ? 2 : 0;
        bits |= query.none.Any() ? 4 : 0;

        switch (bits)
        {
        default:
        case 0:
            break;
        case 1:
            SearchAll(query.all, ms_results);
            break;
        case 2:
            SearchAny(query.any, ms_results);
            break;
        case 4:
            SearchNone(query.none, ms_results);
            break;
        case (1 | 2):
            SearchAllAny(query.all, query.any, ms_results);
            break;
        case (1 | 4):
            SearchAllNone(query.all, query.none, ms_results);
            break;
        case (2 | 4):
            SearchAnyNone(query.any, query.none, ms_results);
            break;
        case (1 | 2 | 4):
            SearchAllAnyNone(query.all, query.any, query.none, ms_results);
            break;
        }

        return ms_results;
    }

    // ------------------------------------------------------------------------

    void Row::Reset()
    {
        if (!IsNull())
        {
            const ComponentType type = m_type;
            const Slice<u16> versions = m_versions;
            u8* pComponents = m_components.begin();
            const i32 count = versions.Size();
            const u32 stride = Component::SizeOf(type);
            for (i32 i = 0; i < count; ++i)
            {
                if (versions[i] != 0)
                {
                    Component::Drop(type, pComponents + i * stride);
                }
            }
        }
        m_versions.Reset();
        m_components.Reset();
        m_type = ComponentType_Null;
    }

    // ------------------------------------------------------------------------

    void Table::Reset()
    {
        m_rowFlags.Clear();
        m_entities.Reset();
        m_names.Reset();
        m_entityFlags.Reset();
        m_versions.Reset();
        m_freelist.Reset();
        for (Row& row : m_rows)
        {
            row.Reset();
        }
    }

    Entity Table::Create(HashString name)
    {
        if (m_freelist.IsEmpty())
        {
            m_freelist.Grow() = m_versions.Size();
            m_versions.Grow() = 0;
            m_entities.Grow() = { TableId_Default, 0, 0 };
            m_names.Grow();
            m_entityFlags.Grow();
        }
        u16 i = m_freelist.PopValue();
        u16 v = (m_versions[i] |= 1u);

        Entity entity;
        entity.table = m_id;
        entity.version = v;
        entity.index = i;

        m_entities[i] = entity;
        m_names[i] = name;
        m_entityFlags[i].Clear();

        return entity;
    }

    bool Table::Destroy(Entity entity)
    {
        Assert(entity.table == m_id);

        if (IsCurrent(entity))
        {
            const u16 e = entity.index;

            for (u32 c = 0; c < ComponentType_Count; ++c)
            {
                m_rows[c].Remove(entity);
            }

            m_names[e] = HashString();
            m_entityFlags[e].Clear();

            m_versions[e]++;
            m_freelist.Grow() = e;

            return true;
        }
        return false;
    }
};
