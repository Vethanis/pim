#pragma once

struct sapp_event;

namespace RenderSystem
{
    void FrameEnd();
    bool OnEvent(const sapp_event* evt);
};
