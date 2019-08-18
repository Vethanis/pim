#include "time_system.h"

#include <sokol/sokol_time.h>

namespace TimeSystem
{
    static u64 ms_appStart;
    static u64 ms_lastTime;
    static u64 ms_dt;
    static f32 ms_dtf32;

    u64 Now() { return stm_now(); }
    u64 StartOfApp() { return ms_appStart; }
    u64 StartOfFrame() { return ms_lastTime; }
    u64 DeltaTime() { return ms_dt; }
    f32 DeltaTimeF32() { return ms_dtf32; }

    f32 ToSeconds(u64 ticks) { return (f32)stm_sec(ticks); }
    f32 ToMilliseconds(u64 ticks) { return (f32)stm_ms(ticks); }
    f32 ToMicroseconds(u64 ticks) { return (f32)stm_us(ticks); }

    void Init()
    {
        stm_setup();
        ms_appStart = Now();
        ms_lastTime = ms_appStart;
        ms_dt = 0;
        ms_dtf32 = 0.0f;
    }
    void Update()
    {
        ms_dt = stm_laptime(&ms_lastTime);
        ms_dtf32 = ToSeconds(ms_dt);
    }
    void Shutdown()
    {

    }
    void Visualize()
    {

    }
};
