#pragma once

#include "common/guid.h"
#include "containers/slice.h"
#include "containers/array.h"
#include <initializer_list>

struct ISystem
{
    ISystem(cstr name, std::initializer_list<cstr> dependencies = {});
    virtual ~ISystem() {}

    virtual void Init() = 0;
    virtual void Update() = 0;
    virtual void Shutdown() = 0;

    Slice<const Guid> GetDependencies() const { return m_dependencies; }

private:
    Array<Guid> m_dependencies;

    ISystem(const ISystem&) = delete;
    ISystem& operator=(const ISystem&) = delete;
};

namespace Systems
{
    void Init();
    void Update();
    void Shutdown();
    ISystem* Find(Guid id);
}
