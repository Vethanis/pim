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

    MaterialId Load(Guid)
    {
        return { 0 };
    }

    void Release(MaterialId)
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
