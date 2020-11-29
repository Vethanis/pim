#pragma once

#include "common/macro.h"

namespace AudioSys
{
    void Init();
    void Update();
    void Shutdown();
    void OnGui(bool* pEnabled);
};
