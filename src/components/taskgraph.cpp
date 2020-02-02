#include "components/taskgraph.h"
#include "containers/hash_dict.h"
#include "components/system.h"

namespace TaskGraph
{
    static HashDict<Guid, TaskNode*, GuidComparator> ms_nodes;

    bool Add(TaskNodeId id, TaskNode* pNode)
    {
        ASSERT(pNode);
        return ms_nodes.Add(id.guid, pNode);
    }

    TaskNode* Remove(TaskNodeId id)
    {
        TaskNode* pNode = nullptr;
        ms_nodes.Remove(id.guid, pNode);
        return pNode;
    }

    TaskNode* Get(TaskNodeId id)
    {
        TaskNode* pNode = nullptr;
        ms_nodes.Get(id.guid, pNode);
        return pNode;
    }

    TaskNodeId Execute()
    {

    }

    void Await(TaskNodeId id)
    {

    }

    void Clear()
    {

    }

    static void Init()
    {
        ms_nodes.Init(Alloc_Pool);
    }
    static void Update()
    {
        Await(Execute());
    }
    static void Shutdown()
    {
        ms_nodes.Reset();
    }

    static constexpr Guid ms_deps[] =
    {
        ToGuid("TaskSystem"),
        ToGuid("Resources"),
    };

    DEFINE_SYSTEM("TaskGraph", { ARGS(ms_deps) }, Init, Update, Shutdown)
};
