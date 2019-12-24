#pragma once

#include "common/guid.h"
#include "rendering/components.h"

namespace MaterialSystem
{
    void Init();
    void Update();
    void Shutdown();

    MaterialId Load(Guid guid);
    void Release(MaterialId id);
};
