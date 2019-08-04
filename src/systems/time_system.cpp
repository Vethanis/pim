#include "time_system.h"

#include <sokol/sokol_time.h>

namespace TimeSystem
{
    static uint64_t ms_lastTime;

    void Init()
    {
        stm_setup();
        ms_lastTime = stm_now();
    }

    float Update()
    {
        uint64_t dt = stm_laptime(&ms_lastTime);
        return (float)stm_sec(dt);
    }
    
    void Shutdown()
    {

    }
};
