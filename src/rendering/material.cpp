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

    MaterialId Load(Guid guid)
    {
        return { 0 };
    }

    void Release(MaterialId id)
    {

    }

    static System ms_system
    {
        "MaterialSystem",
        { "AssetSystem", "RenderSystem" },
        Init,
        Update,
        Shutdown,
    };
};
