#pragma once

struct sapp_event;

struct System
{
    void(*Init)(void);
    void(*Update)(void);
    void(*Shutdown)(void);
    void(*Visualize)(void);
    bool enabled;
    bool visualizing;
};

namespace Systems
{
    void Init();
    void Update();
    void Shutdown();
    void OnEvent(const sapp_event* evt);
    void Exit();
};
