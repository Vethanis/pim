#include "systems/ecs.h"
#include <string.h>

namespace Ecs
{
    static Table ms_chunks[TableId_Count];

    void Init()
    {
        memset(ms_chunks, 0, sizeof(ms_chunks));
    }

    void Shutdown()
    {
        for (Table& chunk : ms_chunks)
        {
            chunk.Reset();
        }
    }

    Table& GetTable(TableId chunkId)
    {
        DebugAssert(chunkId < TableId_Count);
        return ms_chunks[chunkId];
    }

    Table& GetTable(Entity entity)
    {
        return GetTable((TableId)entity.table);
    }

    Slice<Table> Tables()
    {
        return { ms_chunks, TableId_Count };
    }

    // ------------------------------------------------------------------------

    void Row::Reset()
    {
        const DestroyCb onDestroy = m_onDestroy;
        const Slice<u16> versions = m_versions;
        u8* pComponents = m_components.begin();
        const i32 count = versions.size();
        const u32 stride = m_stride;
        for (i32 i = 0; i < count; ++i)
        {
            if (versions[i] != 0)
            {
                onDestroy(pComponents + i * stride);
            }
        }
        m_versions.reset();
        m_components.reset();
    }

    // ------------------------------------------------------------------------

    void Table::Reset()
    {
        m_versions.reset();
        m_freelist.reset();
        for (Row& row : m_rows)
        {
            row.Reset();
        }
    }

    Entity Table::Create(TableId chunkId)
    {
        if (m_freelist.empty())
        {
            m_freelist.grow() = m_versions.size();
            m_versions.grow() = 0;
        }
        u16 i = m_freelist.popValue();
        u16 v = m_versions[i];
        v |= 1;
        m_versions[i] = v;
        Entity entity;
        entity.table = chunkId;
        entity.version = v;
        entity.index = i;
        return entity;
    }

    bool Table::Destroy(Entity entity)
    {
        if (IsCurrent(entity))
        {
            m_freelist.grow() = entity.index;
            m_versions[entity.index]++;
            for (Row& row : m_rows)
            {
                row.Remove(entity);
            }
            return true;
        }
        return false;
    }
};
