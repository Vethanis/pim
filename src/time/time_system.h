#pragma once

#include "common/int_types.h"

namespace TimeSystem
{
    void Init();
    void Update();
    void Shutdown();

    u64 Now();
    u64 StartOfApp();
    u64 StartOfFrame();
    u64 DeltaTime();
    f32 DeltaTimeF32();

    f32 ToSeconds(u64 ticks);
    f32 ToMilliseconds(u64 ticks);
    f32 ToMicroseconds(u64 ticks);
};
