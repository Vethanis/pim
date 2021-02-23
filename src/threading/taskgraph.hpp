#pragma once

#include "common/macro.h"
#include "containers/array.hpp"
#include "containers/dict.hpp"
#include "threading/tasknode.hpp"
#include "threading/event.h"
#include "threading/thread.h"
#include "common/atomics.h"
#include "common/console.h"

class TaskGenerator
{
public:
    virtual Hash32 GetName() = 0;
    virtual TaskNode* Generate() = 0;
    virtual const Array<Hash32>& GetPredecessors() = 0;
};

class TaskGraph final
{
public:

    void Init()
    {
        event_create(&m_waitPush);
        m_barriers.Init();
        store_i32(&m_running, 1, MO_Release);
        store_i32(&m_numRunning, 0, MO_Release);
        store_i32(&m_numSleeping, 0, MO_Release);
        m_threads.Resize(thread_hardware_count() - 1);
        for (thread_t& t : m_threads)
        {
            thread_create(&t, StaticThreadMethod, this);
        }
    }

    void Shutdown()
    {
        store_i32(&m_running, 0, MO_Release);
        while (load_i32(&m_numRunning, MO_Acquire) != 0)
        {
            event_wakeall(&m_waitPush);
        }
        event_destroy(&m_waitPush);
        m_barriers.Shutdown();
        m_generators.Reset();
        m_nodes.Reset();
        m_work.Reset();
        for (thread_t& t : m_threads)
        {
            thread_join(&t);
        }
        m_threads.Reset();
    }

    void Update()
    {
        BuildGraph();
    }

    bool Register(TaskGenerator* generator)
    {
        ASSERT(generator);
        Hash32 name = generator->GetName();
        ASSERT(!name.IsNull());
        for (TaskGenerator* regd : m_generators)
        {
            if (regd->GetName() == name)
            {
                con_logf(LogSev_Warning, "tg", "TaskGenerator '%x' already registered.", name.Value());
                return false;
            }
        }
        m_generators.Grow() = generator;
        return true;
    }

private:

    enum NodeStatus
    {
        NodeStatus_Init = 0,
        NodeStatus_Exec,
        NodeStatus_Complete,
    };

    struct TaskBarrier
    {
        i32 m_status;
        event_t m_event;
        TaskNode* m_owner;

        void Init()
        {
            memset(this, 0, sizeof(*this));
            event_create(&m_event);
        }
        void Shutdown()
        {
            event_destroy(&m_event);
            memset(this, 0, sizeof(*this));
        }
        void Set(TaskNode* node)
        {
            m_owner = node;
            store_i32(&m_status, NodeStatus_Init, MO_Release);
        }
        bool IsComplete() const
        {
            return load_i32(&m_status, MO_Acquire) == NodeStatus_Complete;
        }
        void UpdateStatus()
        {
            ASSERT(m_owner);
            const Array<TaskNode*>& preds = m_owner->m_preds;
            i32 numComplete = 0;
            for (TaskNode* pred : preds)
            {
                ASSERT(pred);
                if (load_i32(&pred->m_status, MO_Acquire) == NodeStatus_Complete)
                {
                    ++numComplete;
                }
            }
            if (numComplete == preds.Size())
            {
                store_i32(&m_status, NodeStatus_Complete, MO_Release);
            }
        }
        void Execute()
        {
            UpdateStatus();
            if (IsComplete())
            {
                event_wakeall(&m_event);
            }
            else
            {
                while (!IsComplete())
                {
                    event_wait(&m_event);
                }
            }
        }
    };

    class BarrierPool
    {
    private:
        i32 m_count;
        i32 m_capacity;
        TaskBarrier* m_barriers;
    public:
        void Init()
        {
            memset(this, 0, sizeof(*this));
        }
        void Shutdown()
        {
            for (i32 i = 0; i < m_capacity; ++i)
            {
                m_barriers[i].Shutdown();
            }
            pim_free(m_barriers);
            memset(this, 0, sizeof(*this));
        }
        i32 Capacity() const { return m_capacity; }
        i32 Size() const { return m_count; }
        void Clear() { m_count = 0; }
        TaskBarrier* begin() { return m_barriers; }
        TaskBarrier* end() { return m_barriers + m_count; }
        TaskBarrier& operator[](i32 i)
        {
            ASSERT((u32)i < (u32)m_count);
            return m_barriers[i];
        }
        void Reserve(i32 ct)
        {
            i32 oldcap = m_capacity;
            if (ct > oldcap)
            {
                i32 newcap = pim_max(oldcap * 2, 16);
                newcap = pim_max(newcap, ct);
                m_barriers = (TaskBarrier*)perm_realloc(m_barriers, sizeof(m_barriers[0]) * newcap);
                for (i32 i = oldcap; i < newcap; ++i)
                {
                    m_barriers[i].Init();
                }
                m_capacity = newcap;
            }
        }
        i32 Add(TaskNode* node)
        {
            i32 i = m_count++;
            Reserve(i + 1);
            m_barriers[i].Set(node);
            return i;
        }
    };

    struct WorkItem
    {
        enum Type
        {
            Type_NULL = 0,
            Type_Node,
            Type_Barrier,
        } type;
        union
        {
            TaskNode* node;
            i32 barrier;
        };
    };

    i32 m_running;
    i32 m_numRunning;
    i32 m_numSleeping;
    event_t m_waitPush;
    BarrierPool m_barriers;
    Array<TaskGenerator*> m_generators;
    Array<TaskNode*> m_nodes;
    Dict<Hash32, TaskNode*> m_lookup;
    Array<WorkItem> m_work;
    Array<thread_t> m_threads;

    void Generate()
    {
        for (TaskNode* node : m_nodes)
        {
            node->~TaskNode();
            pim_free(node);
        }
        const i32 gencount = m_generators.Size();
        m_nodes.Resize(gencount);
        for (i32 i = 0; i < gencount; ++i)
        {
            TaskGenerator* generator = m_generators[i];
            TaskNode* node = generator->Generate();
            if (node)
            {
                Hash32 name = node->GetName();
                ASSERT(!name.IsNull());
                if (!m_lookup.AddCopy(name, node))
                {
                    con_logf(LogSev_Error, "tg", "Duplicate node name detected: %x", name.Value());
                    INTERRUPT();
                }
            }
            m_nodes[i] = node;
        }
        for (i32 i = 0; i < gencount; ++i)
        {
            TaskGenerator* generator = m_generators[i];
            TaskNode* node = m_nodes[i];
            if (node)
            {
                const Array<Hash32>& preds = generator->GetPredecessors();
                node->m_preds.Clear();
                for (Hash32 name : preds)
                {
                    ASSERT(!name.IsNull());
                    TaskNode** pPred = m_lookup.Get(name);
                    if (pPred)
                    {
                        node->m_preds.Grow() = *pPred;
                    }
                    else
                    {
                        // missing predecessor!
                        con_logf(LogSev_Warning, "tg", "Missing predecessor detected: %x", name.Value());
                        ASSERT(false);
                    }
                }
            }
        }
        for (i32 i = gencount - 1; i >= 0; --i)
        {
            if (!m_nodes[i])
            {
                m_nodes.Remove(i);
            }
        }
    }

    void AddNode(TaskNode* node)
    {
        WorkItem& item = m_work.Grow();
        item.type = WorkItem::Type_Node;
        item.node = node;
    }

    void AddBarrier(TaskNode* node)
    {
        i32 barrierCount = m_barriers.Size();
        for (i32 i = barrierCount - 1; i >= 0; --i)
        {
            if (m_barriers[i].m_owner == node)
            {
                return;
            }
        }
        WorkItem& item = m_work.Grow();
        item.type = WorkItem::Type_Barrier;
        item.barrier = m_barriers.Add(node);
    }

    void Sort_Visit(TaskNode* node)
    {
        ASSERT(node);
        ASSERT(node->m_visited != 1); // cyclic
        if (!node->m_visited)
        {
            node->m_visited = 1;
            if (node->m_preds.Size() > 0)
            {
                for (TaskNode* pred : node->m_preds)
                {
                    Sort_Visit(pred);
                }
                AddBarrier(node);
            }
            AddNode(node);
        }
    }

    void Sort()
    {
        m_barriers.Clear();
        m_work.Clear();
        m_work.Reserve(m_nodes.Size());
        for (TaskNode* node : m_nodes)
        {
            node->m_status = 0;
            node->m_head = 0;
            node->m_tail = 0;
            node->m_visited = 0;
        }
        for (TaskNode* node : m_nodes)
        {
            if (!node->m_visited)
            {
                Sort_Visit(node);
            }
        }
    }

    void BuildGraph()
    {
        Generate();
        Sort();
        if (m_work.Size() > 0)
        {
            ASSERT(m_work.back().type == WorkItem::Type_Node);
            AddBarrier(m_work.back().node);
            event_wakeall(&m_waitPush);
            ExecuteGraph();
        }
    }

    struct Range
    {
        i32 a, b;
    };

    static bool StealWork(TaskNode* node, i32 worksize, i32 gran, Range& rangeOut)
    {
        i32 a = fetch_add_i32(&node->m_head, gran, MO_Acquire);
        i32 b = pim_min(a + gran, worksize);
        rangeOut.a = a;
        rangeOut.b = b;
        return a < b;
    }

    static bool UpdateProgress(TaskNode* node, i32 worksize, Range range)
    {
        i32 count = range.b - range.a;
        i32 prev = fetch_add_i32(&node->m_tail, count, MO_Release);
        ASSERT(prev < worksize);
        return (prev + count) >= worksize;
    }

    static void MarkComplete(TaskNode* node)
    {
        store_i32(&node->m_status, NodeStatus_Complete, MO_Release);
    }

    void ExecuteTaskNode(TaskNode* node)
    {
        ASSERT(node);
        i32 tasksplit = m_threads.Size() * m_threads.Size();
        i32 worksize = node->m_worksize;
        i32 gran = worksize / tasksplit;
        gran = pim_max(1, gran);
        Range range;
        while (StealWork(node, worksize, gran, range))
        {
            node->Execute(range.a, range.b);
            if (UpdateProgress(node, worksize, range))
            {
                MarkComplete(node);
            }
        }
    }

    void ExecuteGraph()
    {
        for (const WorkItem& item : m_work)
        {
            switch (item.type)
            {
            default:
                ASSERT(false);
                break;
            case WorkItem::Type_Node:
                ExecuteTaskNode(item.node);
                break;
            case WorkItem::Type_Barrier:
                m_barriers[item.barrier].Execute();
                break;
            }
        }
    }

    void ThreadMethod()
    {
        inc_i32(&m_numRunning, MO_Acquire);
        while (load_i32(&m_running, MO_Acquire))
        {
            inc_i32(&m_numSleeping, MO_Acquire);
            event_wait(&m_waitPush);
            dec_i32(&m_numSleeping, MO_Release);
            ExecuteGraph();
        }
        dec_i32(&m_numRunning, MO_Release);
    }

    static i32 PIM_CDECL StaticThreadMethod(void* arg)
    {
        TaskGraph* self = (TaskGraph*)arg;
        self->ThreadMethod();
        return 0;
    }

};
extern TaskGraph g_TaskGraph;

