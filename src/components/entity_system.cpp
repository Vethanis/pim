#include "components/entity_system.h"
#include "os/thread.h"
#include "components/system.h"
#include "common/find.h"
#include "common/guid.h"
#include "os/atomics.h"
#include "common/time.h"

#include <stdio.h>

IEntitySystem::IEntitySystem(
    cstr name,
    std::initializer_list<cstr> dependencies,
    std::initializer_list<ComponentType> all,
    std::initializer_list<ComponentType> none) :
    ISystem(name, dependencies),
    m_task(this, all, none)
{

}

IEntitySystem::~IEntitySystem()
{
}

static float ms_avg = 0.0f;
constexpr float kAlpha = 1.0f / 255.0f;
static i32 ms_frame = 0;

void IEntitySystem::Update()
{
    u64 a = Time::Now();

    m_task.Setup();
    TaskSystem::Submit(&m_task);
    TaskSystem::Await(&m_task);

    u64 b = Time::Now();
    float ms = Time::ToMilliseconds(b - a);
    ms_avg = (1.0f - kAlpha) * ms_avg + kAlpha * ms;
    ++ms_frame;

    if ((ms_frame & 255) == 0)
    {
        printf("IEntitySystem::Update(): %f ms\n", ms_avg);
    }
}
