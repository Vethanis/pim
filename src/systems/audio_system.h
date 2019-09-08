#pragma once

#include "systems/systems.h"

namespace AudioSystem
{
    void Init();
    void Update();
    void Shutdown();
    void Visualize();

    System GetSystem();
};
