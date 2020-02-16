#include "rendering/material.h"
#include "components/system.h"

namespace MaterialSystem
{
    struct System final : ISystem
    {
        System() : ISystem("MaterialSystem", { "AssetSystem", "RenderSystem" }) {}
        void Init() final
        {

        }
        void Update() final
        {

        }
        void Shutdown() final
        {

        }
    };
    static System ms_system;

    MaterialId Load(Guid guid)
    {
        return { 0 };
    }
    void Release(MaterialId id)
    {

    }
};
