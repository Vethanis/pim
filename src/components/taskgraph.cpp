#include "components/taskgraph.h"
#include "components/system.h"
#include "os/thread.h"
#include "containers/graph.h"

namespace TaskGraph
{
    static OS::Mutex ms_mutex;
    static Graph ms_graph;
    static Array<ITask*> ms_tasks;
    static i32 ms_frame;

    TaskId Add(ITask* pNode)
    {
        ASSERT(pNode);
        OS::LockGuard guard(ms_mutex);
        if (ms_tasks.FindAdd(pNode))
        {
            return { ms_graph.AddVertex(), ms_frame };
        }
        return { -1, -1 };
    }

    bool AddDependency(TaskId src, TaskId dst)
    {
        ASSERT(src.frame == ms_frame);
        ASSERT(dst.frame == ms_frame);
        OS::LockGuard guard(ms_mutex);
        return ms_graph.AddEdge(src.value, dst.value);
    }

    static void Execute()
    {
        OS::LockGuard guard(ms_mutex);

        Slice<const i32> ids = ms_graph.TopoSort();

        for (i32 i : ids)
        {
            for (i32 j : ms_graph.GetEdges(i))
            {
                TaskSystem::Await(ms_tasks[j]);
            }
            TaskSystem::Submit(ms_tasks[i]);
        }

        if (ids.size())
        {
            TaskSystem::Await(ms_tasks[ids.back()]);
        }

        ms_tasks.Clear();
        ++ms_frame;
    }

    static void Init()
    {
        ms_mutex.Open();
        ms_graph.Init(Alloc_Tlsf);
        ms_tasks.Init(Alloc_Tlsf);
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
