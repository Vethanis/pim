#pragma once

#include "common/guid.h"
#include "rendering/components.h"

namespace MeshSystem
{
    MeshId Load(Guid guid);
    void Release(MeshId id);
};
