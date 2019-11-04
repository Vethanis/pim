#pragma once

#include "common/macro.h"
#include "common/int_types.h"
#include "common/guid.h"

struct MeshId
{
    u64 Value;
};

namespace MeshSystem
{
    void Init();
    void Update();
    void Shutdown();

    MeshId Load(Guid guid);
    void Release(MeshId id);
};
