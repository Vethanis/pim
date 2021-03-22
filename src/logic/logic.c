#include "logic/logic.h"
#include "logic/camera_logic.h"
#include "common/profiler.h"

void LogicSys_Init(void)
{
    CameraLogic_Init();
}

ProfileMark(pm_update, LogicSys_Update)
void LogicSys_Update(void)
{
    ProfileBegin(pm_update);

    CameraLogic_Update();

    ProfileEnd(pm_update);
}

void LogicSys_Shutdown(void)
{
    CameraLogic_Shutdown();
}
