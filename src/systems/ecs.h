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

    Table& GetChunk(TableId chunkId);
    Table& GetChunk(Entity entity);
    Slice<Table> Chunks();

    // ------------------------------------------------------------------------

    struct Row
    {
        using RemoveCb = void(*)(void* pComponent);

        Array<u16> m_versions;
        Array<u8> m_components;
        u32 m_stride;
        RemoveCb m_onRemove;

        void Reset();

        inline Slice<const u16> Versions() const
        {
            return m_versions;
        }

        template<typename T>
        inline Slice<const T> Components() const
        {
            DebugAssert(m_stride == sizeof(T));
            return m_components.cast<const T>();
        }

        template<typename T>
        inline Slice<T> Components()
        {
            DebugAssert(m_stride == sizeof(T));
            return m_components.cast<T>();
        }

        inline bool Has(Entity entity) const
        {
            return m_versions.in_range(entity.index) &&
                (m_versions[entity.index] == entity.version);
        }

        template<typename T>
        inline void Add(Entity entity, void(*OnRemove)(T*) = nullptr)
        {
            DebugAssert(entity.IsNotNull());
            if (m_stride == 0)
            {
                m_stride = sizeof(T);
            }
            DebugAssert(m_stride == sizeof(T));
            if (!m_versions.in_range(entity.index))
            {
                m_versions.resize(entity.index + 1);
                m_components.resize((entity.index + 1) * sizeof(T));
            }
            m_versions[entity.index] = entity.version;
            m_components.embed<T>();
            if (OnRemove)
            {
                m_onRemove = (RemoveCb)OnRemove;
            }
        }

        inline bool Remove(Entity entity)
        {
            if (Has(entity))
            {
                DebugAssert(m_stride != 0);
                if (m_onRemove)
                {
                    m_onRemove(m_components.at(entity.index * m_stride));
                }
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
                DebugAssert(m_stride == sizeof(T));
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
        DictTable<8, u16, Row> m_rows;

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
        inline bool HasType() const
        {
            return m_rows.find(TypeOf(T)) != -1;
        }

        template<typename T>
        inline Row& GetRow()
        {
            return m_rows[TypeOf(T)];
        }
    };

    // ------------------------------------------------------------------------
};
