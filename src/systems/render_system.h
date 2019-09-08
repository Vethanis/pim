#pragma once

#include "systems/systems.h"

struct sapp_event;

namespace RenderSystem
{
    void Init();
    void Update();
    void FrameEnd();
    void Shutdown();

    bool OnEvent(const sapp_event* evt);

    void Visualize();

    System GetSystem();
};
