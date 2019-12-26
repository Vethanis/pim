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

    static constexpr Guid ms_dependencies[] =
    {
        ToGuid("AssetSystem"),
        ToGuid("RenderSystem"),
    };

    static constexpr System ms_system =
    {
        ToGuid("MeshSystem"),
        { ARGS(ms_dependencies) },
        Init,
        Update,
        Shutdown,
    };
    static RegisterSystem ms_register(ms_system);

    MeshId Load(Guid guid)
    {
        return { 0 };
    }
    void Release(MeshId id)
    {

    }
};
