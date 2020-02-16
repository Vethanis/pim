#include "rendering/mesh.h"
#include "components/system.h"

namespace MeshSystem
{
    struct System final : ISystem
    {
        System() : ISystem("MeshSystem", { "AssetSystem", "RenderSystem" }) {}
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

    MeshId Load(Guid guid)
    {
        return {};
    }
    void Release(MeshId id)
    {

    }
};
