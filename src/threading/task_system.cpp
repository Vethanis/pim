#include "threading/task_system.h"
#include "os/thread.h"
#include "components/system.h"

namespace TaskSystem
{
    static void Init()
    {

    }

    static void Update()
    {

    }

    static void Shutdown()
    {

    }

    static constexpr System ms_system =
    {
        ToGuid("TaskSystem"),
        { 0, 0 },
        Init,
        Update,
        Shutdown,
    };
    static RegisterSystem ms_register(ms_system);
};
