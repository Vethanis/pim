#include "threading/taskgraph.h"
#include "components/system.h"
#include "os/thread.h"
#include "os/atomics.h"
#include "containers/graph.h"
#include"components/ecs.h"

namespace TaskGraph
{
    static OS::Mutex ms_mutex;
    static Graph ms_graph;
    static Array<TaskNode*> ms_nodes;

    void AddVertex(TaskNode* pNode)
    {
        ASSERT(pNode);
        OS::LockGuard guard(ms_mutex);
        i32 i = ms_nodes.PushBack(pNode);
        i32 j = ms_graph.AddVertex();
        ASSERT(i == j);
        pNode->m_graphId = j;
    }

    void AddVertices(Slice<TaskNode*> nodes)
    {
        OS::LockGuard guard(ms_mutex);
        for (TaskNode* pNode : nodes)
        {
            ASSERT(pNode);
            i32 i = ms_nodes.PushBack(pNode);
            i32 j = ms_graph.AddVertex();
            ASSERT(i == j);
            pNode->m_graphId = j;
        }
    }

    bool AddEdge(TaskNode* src, TaskNode* dst)
    {
        ASSERT(src);
        ASSERT(dst);
        OS::LockGuard guard(ms_mutex);
        return ms_graph.AddEdge(src->m_graphId, dst->m_graphId);
    }

    void Evaluate()
    {
        OS::LockGuard guard(ms_mutex);
        ECS::SetPhase(ECS::Phase_MultiThread);

        ms_graph.Sort();
        for (i32 i : ms_graph)
        {
            for (i32 j : ms_graph.GetEdges(i))
            {
                TaskSystem::Await(ms_nodes[j]);
            }
            ms_nodes[i]->BeforeSubmit();
            TaskSystem::Submit(ms_nodes[i]);
        }
        for (i32 i : ms_graph)
        {
            TaskSystem::Await(ms_nodes[i]);
        }
        ms_graph.Clear();
        ms_nodes.Clear();
        ECS::SetPhase(ECS::Phase_MainThread);
    }

    struct System final : ISystem
    {
        System() : ISystem("TaskGraph", { "TaskSystem" }) {}
        void Init() final
        {
            ms_mutex.Open();
            ms_graph.Init();
            ms_nodes.Init();
        }
        void Update() final {}
        void Shutdown() final
        {
            ms_mutex.Lock();
            ms_graph.Reset();
            ms_nodes.Reset();
            ms_mutex.Close();
        }
    };

    static System ms_system;
};
