#include "threading/taskgraph.h"
#include "components/system.h"
#include "os/thread.h"
#include "os/atomics.h"
#include "containers/graph.h"

static OS::Mutex ms_mutex;
static Array<TaskNode*> ms_tasks;
static Array<TaskNode*> ms_list;
static i32 ms_frame;

struct TaskGraphImpl
{
    static bool AddEdge(TaskNode* src, TaskNode* dst)
    {
        ASSERT(src);
        ASSERT(dst);
        return dst->m_edges.FindAdd(src);
    }

    static bool RmEdge(TaskNode* src, TaskNode* dst)
    {
        ASSERT(src);
        ASSERT(dst);
        return dst->m_edges.FindRemove(src);
    }

    static void ClearEdges(TaskNode* dst)
    {
        ASSERT(dst);
        dst->m_edges.Clear();
    }

    static void Visit(TaskNode* pNode)
    {
        ASSERT(pNode->m_mark != 1);
        if (pNode->m_mark == 0)
        {
            pNode->m_mark = 1;
            for (TaskNode* pDep : pNode->m_edges)
            {
                Visit(pDep);
            }
            pNode->m_mark = 2;
            ms_list.PushBack(pNode);
        }
    }

    static void Evaluate()
    {
        OS::LockGuard guard(ms_mutex);

        for (TaskNode* pNode : ms_tasks)
        {
            pNode->m_mark = 0;
        }

        ms_list.Clear();
        for (TaskNode* pNode : ms_tasks)
        {
            if (pNode->m_mark == 0)
            {
                Visit(pNode);
            }
        }
        ms_tasks.Clear();

        for (TaskNode* pNode : ms_list)
        {
            for (TaskNode* pDep : pNode->m_edges)
            {
                TaskSystem::Await(pDep);
            }
            TaskSystem::Submit(pNode);
        }

        if (ms_list.size())
        {
            TaskNode* pLast = ms_list.back();
            TaskSystem::Await(pLast);
        }
        ms_list.Clear();
    }
};

namespace TaskGraph
{
    void AddVertex(TaskNode* pNode)
    {
        ASSERT(pNode);
        OS::LockGuard guard(ms_mutex);
        ms_tasks.PushBack(pNode);
    }

    void AddVertices(Slice<TaskNode*> nodes)
    {
        OS::LockGuard guard(ms_mutex);
        for (TaskNode* pNode : nodes)
        {
            ASSERT(pNode);
            ms_tasks.PushBack(pNode);
        }
    }

    bool AddEdge(TaskNode* src, TaskNode* dst)
    {
        ASSERT(src);
        ASSERT(dst);
        return TaskGraphImpl::AddEdge(src, dst);
    }

    bool RmEdge(TaskNode* src, TaskNode* dst)
    {
        ASSERT(src);
        ASSERT(dst);
        return TaskGraphImpl::RmEdge(src, dst);
    }

    void ClearEdges(TaskNode* dst)
    {
        ASSERT(dst);
        TaskGraphImpl::ClearEdges(dst);
    }

    void Evaluate()
    {
        TaskGraphImpl::Evaluate();
    }

    struct System final : ISystem
    {
        System() : ISystem("TaskGraph", { "TaskSystem" }) {}
        void Init() final
        {
            ms_mutex.Open();
            ms_list.Init(Alloc_Tlsf);
            ms_tasks.Init(Alloc_Tlsf);
        }
        void Update() final {}
        void Shutdown() final
        {
            ms_mutex.Lock();
            ms_list.Reset();
            ms_tasks.Reset();
            ms_mutex.Close();
        }
    };

    static System ms_system;
};
