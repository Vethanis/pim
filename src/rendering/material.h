#pragma once

#include "common/guid.h"
#include "rendering/components.h"

namespace MaterialSystem
{
    MaterialId Load(Guid guid);
    void Release(MaterialId id);
};
