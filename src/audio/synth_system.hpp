#pragma once

#include "common/macro.h"

namespace SynthSys
{
    void Init();
    void Update();
    void Shutdown();
    void OnGui();
    void OnSample(float *const pim_noalias buffer, i32 frames, i32 channels);
};
