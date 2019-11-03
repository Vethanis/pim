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

    Table& GetChunk(TableId chunkId)
    {
        DebugAssert(chunkId < TableId_Count);
        return ms_chunks[chunkId];
    }

    Table& GetChunk(Entity entity)
    {
        return GetChunk((TableId)entity.table);
    }

    Slice<Table> Chunks()
    {
        return { ms_chunks, TableId_Count };
    }

    // ------------------------------------------------------------------------

    void Row::Reset()
    {
        RemoveCb onRemove = m_onRemove;
        Slice<u16> versions = m_versions;
        u8* pComponents = m_components.begin();
        i32 count = versions.size();
        const i32 stride = m_stride;
        for (i32 i = 0; i < count; ++i)
        {
            if (versions[i] != 0)
            {
                onRemove(pComponents + i * stride);
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
        for (Dict<u16, Row>& dict : m_rows.m_dicts)
        {
            for (Row& row : dict.m_values)
            {
                row.Reset();
            }
        }
        m_rows.reset();
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
            for (Dict<u16, Row>& dict : m_rows.m_dicts)
            {
                for (Row& row : dict.m_values)
                {
                    row.Remove(entity);
                }
            }
            return true;
        }
        return false;
    }
};
