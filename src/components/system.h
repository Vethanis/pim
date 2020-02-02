#pragma once

#include "common/guid.h"
#include "containers/slice.h"

struct System
{
    Guid Name;
    Slice<const Guid> Dependencies;
    void(*Init)(void);
    void(*Update)(void);
    void(*Shutdown)(void);
};

namespace SystemRegistry
{
    void Register(System system);

    void Init();
    void Update();
    void Shutdown();
}

struct RegisterSystem
{
    RegisterSystem(System system)
    {
        SystemRegistry::Register(system);
    }
};

#define DEFINE_SYSTEM(name, deps, initFn, updateFn, shutdownFn) \
    static constexpr System ms_system = \
    { ToGuid(name), deps, initFn, updateFn, shutdownFn }; \
    static RegisterSystem ms_register(ms_system);
