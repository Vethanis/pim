#pragma once

struct sapp_event;

namespace RenderSystem
{
    void Init();
    void Shutdown();

    void BeginDraws();
    void EndDraws();

    void BeginVisualize();
    void EndVisualize();

    bool OnEvent(const sapp_event* evt);

    void Visualize();
};
