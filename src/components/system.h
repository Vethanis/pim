#pragma once

#include "common/int_types.h"
#include <initializer_list>

struct System
{
    System() {}
    System(
        cstr name,
        std::initializer_list<cstr> dependencies,
        void(*pInit)(),
        void(*pUpdate)(),
        void(*pShutdown)());

    cstr m_name;
    std::initializer_list<cstr> m_dependencies;
    void(*Init)();
    void(*Update)();
    void(*Shutdown)();
};

namespace Systems
{
    void Init();
    void Update();
    void Shutdown();
}
