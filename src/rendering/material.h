#pragma once

#include "common/macro.h"
#include "common/int_types.h"
#include "common/guid.h"

struct MaterialId
{
    u64 Value;
};

namespace MaterialSystem
{
    void Init();
    void Update();
    void Shutdown();

    MaterialId Load(Guid guid);
    void Release(MaterialId id);
};
