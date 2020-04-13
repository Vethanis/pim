#include "logic/logic.h"
#include "logic/camera_logic.h"

void logic_sys_init(void)
{
    camera_logic_init();
}

void logic_sys_update(void)
{
    camera_logic_update();
}

void logic_sys_shutdown(void)
{
    camera_logic_shutdown();
}
