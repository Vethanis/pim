#include "time_system.h"

#include <sokol/sokol_time.h>

static uint64_t ms_lastTime;

void timesys_init()
{
    stm_setup();
    ms_lastTime = stm_now();
}

float timesys_update()
{
    uint64_t dt = stm_laptime(&ms_lastTime);
    return (float)stm_sec(dt);
}

void timesys_shutdown()
{

}
