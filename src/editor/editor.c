#include "editor/editor.h"
#include "editor/menubar.h"
#include "common/profiler.h"

// ----------------------------------------------------------------------------

void editor_sys_init(void)
{
    menubar_init();
}

ProfileMark(pm_update, editor_sys_update)
void editor_sys_update(void)
{
    ProfileBegin(pm_update);

    menubar_update();

    ProfileEnd(pm_update);
}

void editor_sys_shutdown(void)
{
    menubar_shutdown();
}

// ----------------------------------------------------------------------------
