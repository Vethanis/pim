#include "tables/entity_table.h"
#include "common/array.h"
#include "common/dict.h"
#include "common/reflection.h"

static constexpr AllocatorType TableAllocator = Allocator_Malloc;
static constexpr u32 TableWidth = 256;

static void IncrementVersion(u32& x)
{
    x = (x + 2u) & (~1u);
}

struct ComponentArray
{
    Array<u32> versions;
    Array<u8> data;
};

struct ComponentArrays
{
    u32 m_stride;
    void(*m_onAdd)(void*);
    void(*m_onRemove)(void*);
    ComponentArray m_arrays[TableWidth];

    inline void Init(
        void(*OnAdd)(void*),
        void(*OnRemove)(void*),
        u32 stride)
    {
        m_onAdd = OnAdd;
        m_onRemove = OnRemove;
        m_stride = stride;
        for (ComponentArray& x : m_arrays)
        {
            x.versions.init(TableAllocator);
            x.data.init(TableAllocator);
        }
    }
    inline void Reset()
    {
        for (ComponentArray& x : m_arrays)
        {
            x.versions.reset();
            x.data.reset();
        }
    }

    inline Slice<const u32> GetVersions(u8 table)
    {
        auto& x = m_arrays[table].versions;
        return { x.begin(), x.size() };
    }
    inline Slice<u8> GetData(u8 table)
    {
        auto& x = m_arrays[table].data;
        return { x.begin(), x.size() };
    }

    inline bool Has(Entity e) const
    {
        const ComponentArray& carr = m_arrays[e.table];
        return (e.id < carr.versions.size()) && (e.version == carr.versions[e.id]);
    }
    inline void* _Get(Entity e)
    {
        return m_arrays[e.table].data.begin() + (usize)e.id * (usize)m_stride;
    }
    inline void* Get(Entity e)
    {
        return Has(e) ? _Get(e) : 0;
    }
    inline void Add(Entity e)
    {
        auto& versions = m_arrays[e.table].versions;
        auto& data = m_arrays[e.table].data;

        const i32 oldSize = versions.size();
        if ((e.id >= oldSize) || (versions[e.id] < e.version))
        {
            if (e.id >= oldSize)
            {
                versions.resize(e.id + 1);
                data.resize((e.id + 1) * m_stride);
            }
            versions[e.id] = e.version;
            if (m_onAdd)
            {
                m_onAdd(_Get(e));
            }
        }
    }
    inline void Remove(Entity e)
    {
        if (!Has(e))
        {
            return;
        }
        IncrementVersion(m_arrays[e.table].versions[e.id]);
        if (m_onRemove)
        {
            m_onRemove(_Get(e));
        }
    }
};

namespace EntityTable
{
    static Array<u32> ms_versions[TableWidth];
    static Array<u16> ms_freelists[TableWidth];
    static Array<usize> ms_registrations;

    static inline void SetArrays(usize typeId, ComponentArrays* carrs)
    {
        RfGetItems(typeId).compArrays = carrs;
    }
    static inline ComponentArrays* GetArrays(usize typeId)
    {
        return (ComponentArrays*)(RfGetItems(typeId).compArrays);
    }
    static inline bool SAlive(Entity ent)
    {
        const u32 * const versions = ms_versions[ent.table].m_ptr;
        const i32 size = ms_versions[ent.table].m_len;
        return (ent.version & 1u) &&
            (ent.id < size) &&
            (versions[ent.id] == ent.version);
    }

    void Init()
    {
        for (auto& versions : ms_versions)
        {
            versions.init(TableAllocator);
        }
        for (auto& freelist : ms_freelists)
        {
            freelist.init(TableAllocator);
        }
        ms_registrations.m_alloc = TableAllocator;
    }
    void Shutdown()
    {
        for (auto& versions : ms_versions)
        {
            versions.reset();
        }
        for (auto& freelist : ms_freelists)
        {
            freelist.reset();
        }
        for(usize typeId : ms_registrations)
        {
            ComponentArrays* compArrays = GetArrays(typeId);
            compArrays->Reset();
        }
    }

    void Register(
        usize typeId,
        void(*OnAdd)(void*),
        void(*OnRemove)(void*))
    {
        ComponentArrays* carrs = GetArrays(typeId);
        if (!carrs)
        {
            carrs = Allocator::Alloc<ComponentArrays>(TableAllocator, 1);
            MemClear(carrs, sizeof(ComponentArrays));
            carrs->Init(OnAdd, OnRemove, RfGet(typeId).size);
            SetArrays(typeId, carrs);
            ms_registrations.grow() = typeId;
        }
    }

    Slice<const u32> EntVersions(u8 i)
    {
        return { ms_versions[i].begin(), ms_versions[i].size() };
    }
    Slice<const u32> CompVersions(u8 i, usize typeId)
    {
        ComponentArrays* carrs = GetArrays(typeId);
        DebugAssert(carrs);
        return carrs->GetVersions(i);
    }
    Slice<u8> CompData(u8 i, usize typeId)
    {
        ComponentArrays* carrs = GetArrays(typeId);
        DebugAssert(carrs);
        return carrs->GetData(i);
    }

    bool Alive(Entity ent)
    {
        return SAlive(ent);
    }
    Entity Create(u8 i)
    {
        Array<u32>& version = ms_versions[i];
        Array<u16>& freelist = ms_freelists[i];

        if (freelist.empty())
        {
            DebugAssert(version.size() < 0xFFFF);
            freelist.grow() = (u16)version.size();
            version.grow() = 0u;
        }

        Entity ent;
        ent.table = i;
        ent.id = freelist.back();
        freelist.pop();
        version[ent.id] |= 1u;
        ent.version = version[ent.id];

        return ent;
    }
    void Destroy(Entity e)
    {
        if (SAlive(e))
        {
            IncrementVersion(ms_versions[e.table][e.id]);
            ms_freelists[e.table].grow() = e.id;
            for (usize typeId : ms_registrations)
            {
                ComponentArrays* carrs = GetArrays(typeId);
                DebugAssert(carrs);
                carrs->Remove(e);
            }
        }
    }

    void Add(Entity e, usize typeId)
    {
        if (SAlive(e))
        {
            ComponentArrays* carrs = GetArrays(typeId);
            DebugAssert(carrs);
            carrs->Add(e);
        }
    }
    void Remove(Entity e, usize typeId)
    {
        if (SAlive(e))
        {
            ComponentArrays* carrs = GetArrays(typeId);
            DebugAssert(carrs);
            carrs->Remove(e);
        }
    }
    void* Get(Entity e, usize typeId)
    {
        if (SAlive(e))
        {
            ComponentArrays* carrs = GetArrays(typeId);
            DebugAssert(carrs);
            return carrs->Get(e);
        }
        return 0;
    }

    void Visualize()
    {

    }
};
