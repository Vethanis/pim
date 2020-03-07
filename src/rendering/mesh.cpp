#include "rendering/mesh.h"
#include "components/system.h"

namespace MeshSystem
{
    static void Init()
    {

    }

    static void Update()
    {

    }

    static void Shutdown()
    {

    }

    MeshId Load(Guid)
    {
        return {};
    }

    void Release(MeshId)
    {

    }

    static System ms_system
    {
        "MeshSystem", { "AssetSystem", "RenderSystem" }, Init, Update, Shutdown,
    };
};
