#include "components/taskgraph.h"
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
    static void ClearMark(TaskNode* pNode)
    {
        ASSERT(pNode);
        Store(pNode->m_mark, 0);
    }

    static void MarkTemp(TaskNode* pNode)
    {
        ASSERT(pNode);
        Store(pNode->m_mark, 1);
    }

    static void MarkPerm(TaskNode* pNode)
    {
        ASSERT(pNode);
        Store(pNode->m_mark, 2);
    }

    static i32 GetMark(TaskNode* pNode)
    {
        ASSERT(pNode);
        return Load(pNode->m_mark);
    }

    static bool IsUnmarked(TaskNode* pNode)
    {
        return GetMark(pNode) == 0;
    }

    static bool IsTemp(TaskNode* pNode)
    {
        return GetMark(pNode) == 1;
    }

    static bool IsPerm(TaskNode* pNode)
    {
        return GetMark(pNode) == 2;
    }

    static Slice<TaskNode*> GetEdges(TaskNode* pNode)
    {
        ASSERT(pNode);
        return pNode->m_edges;
    }

    static void AddVertex(TaskNode* pNode)
    {
        ASSERT(pNode);
        OS::LockGuard guard(ms_mutex);
        ms_tasks.PushBack(pNode);
    }

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

    static void Visit(TaskNode* pNode)
    {
        ASSERT(!IsTemp(pNode));
        if (IsUnmarked(pNode))
        {
            MarkTemp(pNode);
            Slice<TaskNode*> edges = GetEdges(pNode);
            for (TaskNode* pDep : edges)
            {
                Visit(pDep);
            }
            MarkPerm(pNode);
            ms_list.PushBack(pNode);
        }
    }

    static void Evaluate()
    {
        OS::LockGuard guard(ms_mutex);

        for (TaskNode* pNode : ms_tasks)
        {
            ClearMark(pNode);
        }

        ms_list.Clear();
        for (TaskNode* pNode : ms_tasks)
        {
            if (IsUnmarked(pNode))
            {
                Visit(pNode);
            }
        }

        for (TaskNode* pNode : ms_list)
        {
            Slice<TaskNode*> edges = GetEdges(pNode);
            for (TaskNode* pDep : edges)
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
        TaskGraphImpl::AddVertex(pNode);
    }

    bool AddEdge(TaskNode* src, TaskNode* dst)
    {
        return TaskGraphImpl::AddEdge(src, dst);
    }

    bool RmEdge(TaskNode* src, TaskNode* dst)
    {
        return TaskGraphImpl::RmEdge(src, dst);
    }

    void Evaluate()
    {
        TaskGraphImpl::Evaluate();
    }

    static void Init()
    {
        ms_mutex.Open();
        ms_list.Init(Alloc_Tlsf);
        ms_tasks.Init(Alloc_Tlsf);
    }

    static void Update()
    {

    }

    static void Shutdown()
    {
        ms_mutex.Close();
        ms_list.Reset();
        ms_tasks.Reset();
    }
};

static constexpr Guid ms_deps[] =
{
    ToGuid("TaskSystem"),
};

DEFINE_SYSTEM("TaskGraph", { ARGS(ms_deps) }, TaskGraph::Init, TaskGraph::Update, TaskGraph::Shutdown)
