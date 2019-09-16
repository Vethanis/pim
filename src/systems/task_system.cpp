#include "systems/task_system.h"
#include "os/thread.h"

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

    static void Visualize()
    {

    }

    System GetSystem()
    {
        System sys = {};
        sys.Init = Init;
        sys.Update = Update;
        sys.Shutdown = Shutdown;
        sys.Visualize = Visualize;
        sys.enabled = true;
        sys.visualizing = false;
        return sys;
    }
};
