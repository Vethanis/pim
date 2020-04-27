#include "editor/editor.h"
#include "editor/menubar.h"
#include "common/profiler.h"
#include "editor/console.h"

// ----------------------------------------------------------------------------

void editor_sys_init(void)
{
    menubar_init();
    con_sys_init();
}

ProfileMark(pm_update, editor_sys_update)
void editor_sys_update(void)
{
    ProfileBegin(pm_update);

    con_sys_update();
    menubar_update();

    ProfileEnd(pm_update);
}

void editor_sys_shutdown(void)
{
    con_sys_shutdown();
    menubar_shutdown();
}

// ----------------------------------------------------------------------------
