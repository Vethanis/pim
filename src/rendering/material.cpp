#include "rendering/material.h"
#include "components/system.h"

namespace MaterialSystem
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
        ToGuid("MaterialSystem"),
        { ARGS(ms_dependencies) },
        Init,
        Update,
        Shutdown,
    };
    static RegisterSystem ms_register(ms_system);

    MaterialId Load(Guid guid)
    {
        return { 0 };
    }
    void Release(MaterialId id)
    {

    }
};
