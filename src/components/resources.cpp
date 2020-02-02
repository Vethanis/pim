#include "components/resources.h"
#include "containers/hash_dict.h"
#include "components/system.h"

namespace Resources
{
    static HashDict<Guid, IResource*, GuidComparator> ms_resources;

    bool Create(ResourceId id, IResource* pResource)
    {
        ASSERT(pResource);
        return ms_resources.Add(id.guid, pResource);
    }

    IResource* Destroy(ResourceId id)
    {
        IResource* pResource = nullptr;
        ms_resources.Remove(id.guid, pResource);
        return pResource;
    }

    IResource* Get(ResourceId id)
    {
        IResource* pResource = nullptr;
        ms_resources.Get(id.guid, pResource);
    }

    static void Init()
    {
        ms_resources.Init(Alloc_Pool);
    }
    static void Update()
    {

    }
    static void Shutdown()
    {
        ms_resources.Reset();
    }

    DEFINE_SYSTEM("Resources", {}, Init, Update, Shutdown)
};
