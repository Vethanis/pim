#pragma once

#include "components/entity.h"
#include "components/component_id.h"
#include "allocator/allocator.h"
#include "containers/slice.h"
#include "containers/array.h"
#include "containers/bitfield.h"
#include "threading/task.h"
#include <initializer_list>

namespace ECS
{
    static constexpr i32 kMaxTypes = 256;
    using TypeFlags = BitField<ComponentId, kMaxTypes>;

    i32 EntityCount();

    Entity Create(std::initializer_list<ComponentId> components);
    bool Destroy(Entity entity);
    bool IsCurrent(Entity entity);

    bool HasAll(Entity entity, TypeFlags all);
    bool HasAny(Entity entity, TypeFlags any);
    bool HasNone(Entity entity, TypeFlags none);

    struct Row
    {
        ComponentId type;
        i32 stride;
        void* ptr;
    };

    struct OnSlabTask : ITask
    {
        OnSlabTask() : ITask(0, 0, 1) {}

        void SetQuery(TypeFlags all, TypeFlags none);
        void Execute(i32, i32) final;
        virtual void OnSlab(i32 length, Slice<Row> rows) = 0;
        static i32 FindRow(ComponentId type, Slice<const Row> rows);

        template<typename T>
        static Slice<T> GetRow(ComponentId id, i32 length, Slice<Row> rows)
        {
            ASSERT(GetId<T>() == id);
            const i32 i = FindRow(id, rows);
            ASSERT(i != -1);
            return { (T*)(rows[i].ptr), length };
        }

    private:
        TypeFlags m_all;
        TypeFlags m_none;
    };

    template<typename T0>
    struct ForEach1 : OnSlabTask
    {
        ForEach1() :
            OnSlabTask(),
            m_idE(GetId<Entity>()),
            m_id0(GetId<T0>()) {}
        void OnSlab(i32 length, Slice<Row> rows) final
        {
            OnRows(
                GetRow<Entity>(m_idE, length, rows),
                GetRow<T0>(m_id0, length, rows));
        }
        virtual void OnRows(
            Slice<const Entity> entities,
            Slice<T0> t0s) = 0;
    private:
        ComponentId m_idE;
        ComponentId m_id0;
    };

    template<typename T0, typename T1>
    struct ForEach2 : OnSlabTask
    {
        ForEach2() :
            OnSlabTask(),
            m_idE(GetId<Entity>()),
            m_id0(GetId<T0>()),
            m_id1(GetId<T1>()) {}
        void OnSlab(i32 length, Slice<Row> rows) final
        {
            OnRows(
                GetRow<Entity>(m_idE, length, rows),
                GetRow<T0>(m_id0, length, rows),
                GetRow<T1>(m_id1, length, rows));
        }
        virtual void OnRows(
            Slice<const Entity> entities,
            Slice<T0> t0s,
            Slice<T1> t1s) = 0;
    private:
        ComponentId m_idE;
        ComponentId m_id0;
        ComponentId m_id1;
    };

    template<typename T0, typename T1, typename T2>
    struct ForEach3 : OnSlabTask
    {
        ForEach3() :
            OnSlabTask(),
            m_idE(GetId<Entity>()),
            m_id0(GetId<T0>()),
            m_id1(GetId<T1>()),
            m_id2(GetId<T2>()) {}
        void OnSlab(i32 length, Slice<Row> rows) final
        {
            OnRows(
                GetRow<Entity>(m_idE, length, rows),
                GetRow<T0>(m_id0, length, rows),
                GetRow<T1>(m_id1, length, rows),
                GetRow<T2>(m_id2, length, rows));
        }
        virtual void OnRows(
            Slice<const Entity> entities,
            Slice<T0> t0s,
            Slice<T1> t1s,
            Slice<T2> t2s) = 0;
    private:
        ComponentId m_idE;
        ComponentId m_id0;
        ComponentId m_id1;
        ComponentId m_id2;
    };

    template<typename T0, typename T1, typename T2, typename T3>
    struct ForEach4 : OnSlabTask
    {
        ForEach4() :
            OnSlabTask(),
            m_idE(GetId<Entity>()),
            m_id0(GetId<T0>()),
            m_id1(GetId<T1>()),
            m_id2(GetId<T2>()),
            m_id3(GetId<T3>()) {}
        void OnSlab(i32 length, Slice<Row> rows) final
        {
            OnRows(
                GetRow<Entity>(m_idE, length, rows),
                GetRow<T0>(m_id0, length, rows),
                GetRow<T1>(m_id1, length, rows),
                GetRow<T2>(m_id2, length, rows),
                GetRow<T3>(m_id3, length, rows));
        }
        virtual void OnRows(
            Slice<const Entity> entities,
            Slice<T0> t0s,
            Slice<T1> t1s,
            Slice<T2> t2s,
            Slice<T3> t3s) = 0;
    private:
        ComponentId m_idE;
        ComponentId m_id0;
        ComponentId m_id1;
        ComponentId m_id2;
        ComponentId m_id3;
    };
};
