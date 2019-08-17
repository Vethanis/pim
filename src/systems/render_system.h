#pragma once

struct sapp_event;

namespace RenderSystem
{
    void Init();
    void BeginFrame();
    void EndFrame();
    void Shutdown();
    bool OnEvent(const sapp_event* evt);
};
