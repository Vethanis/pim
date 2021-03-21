#include "logic/logic.h"
#include "logic/camera_logic.h"
#include "common/profiler.h"

void LogicSys_Init(void)
{
    camera_logic_init();
}

ProfileMark(pm_update, LogicSys_Update)
void LogicSys_Update(void)
{
    ProfileBegin(pm_update);

    camera_logic_update();

    ProfileEnd(pm_update);
}

void LogicSys_Shutdown(void)
{
    camera_logic_shutdown();
}
