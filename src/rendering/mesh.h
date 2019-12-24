#pragma once

#include "common/guid.h"
#include "rendering/components.h"

namespace MeshSystem
{
    void Init();
    void Update();
    void Shutdown();

    MeshId Load(Guid guid);
    void Release(MeshId id);
};
