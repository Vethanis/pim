#pragma once

struct sapp_event;

namespace RenderSystem
{
    void Init();
    void NewFrame(float dt);
    void Update(float dt);
    void Shutdown();
    bool OnEvent(const sapp_event* evt);
};
