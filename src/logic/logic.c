#include "logic/logic.h"
#include "logic/camera_logic.h"
#include "common/profiler.h"

void logic_sys_init(void)
{
    camera_logic_init();
}

ProfileMark(pm_update, logic_sys_update)
void logic_sys_update(void)
{
    ProfileBegin(pm_update);

    camera_logic_update();

    ProfileEnd(pm_update);
}

void logic_sys_shutdown(void)
{
    camera_logic_shutdown();
}
