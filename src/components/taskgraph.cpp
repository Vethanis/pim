#include "components/taskgraph.h"
#include "components/system.h"
#include "os/thread.h"

static constexpr u32 kNoMark = 0;
static constexpr u32 kTempMark = 1 << 0;
static constexpr u32 kPermMark = 1 << 1;

namespace TaskGraph
{
    static OS::Mutex ms_mutex;
    static Array<TaskNode*> ms_graph;

    bool Add(TaskNode* pNode)
    {
        ASSERT(pNode);
        OS::LockGuard guard(ms_mutex);
        return ms_graph.FindAdd(pNode);
    }

    bool AddDependency(TaskNode* pFirst, TaskNode* pAfter)
    {
        ASSERT(pFirst);
        ASSERT(pAfter);
        return pAfter->m_inward.FindAdd(pFirst);
    }

    bool RmDependency(TaskNode* pFirst, TaskNode* pAfter)
    {
        ASSERT(pFirst);
        ASSERT(pAfter);
        return pAfter->m_inward.FindRemove(pFirst);
    }

    static void Visit(Array<TaskNode*>& list, TaskNode* n)
    {
        u32& mark = n->m_graphMark;
        ASSERT(mark != kTempMark);
        if (mark == kNoMark)
        {
            mark = kTempMark;
            for (TaskNode* m : n->m_inward)
            {
                Visit(list, m);
            }
            mark = kPermMark;
            list.PushBack(n);
        }
    }

    static void Execute()
    {
        OS::LockGuard guard(ms_mutex);

        Array<TaskNode*> list = CreateArray<TaskNode*>(Alloc_Linear, ms_graph.size());

        for (TaskNode* node : ms_graph)
        {
            node->m_graphMark = 0;
        }

        for (TaskNode* node : ms_graph)
        {
            if (node->m_graphMark == kNoMark)
            {
                Visit(list, node);
            }
        }

        TaskNode* pLast = nullptr;
        for (TaskNode* pNode : list)
        {
            pNode = list.PopBack();
            for (ITask* runsFirst : pNode->m_inward)
            {
                TaskSystem::Await(runsFirst);
            }
            TaskSystem::Submit(pNode);
            pLast = pNode;
        }

        list.Reset();

        if (pLast)
        {
            TaskSystem::Await(pLast);
        }

        ms_graph.Clear();
    }

    static void Init()
    {
        ms_mutex.Open();
        ms_graph.Init(Alloc_Pool);
    }
    static void Update()
    {
        Execute();
    }
    static void Shutdown()
    {
        ms_mutex.Close();
        ms_graph.Reset();
    }

    static constexpr Guid ms_deps[] =
    {
        ToGuid("TaskSystem"),
    };

    DEFINE_SYSTEM("TaskGraph", { ARGS(ms_deps) }, Init, Update, Shutdown)
};
