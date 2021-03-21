#include "editor/editor.h"
#include "editor/menubar.h"
#include "common/profiler.h"

// ----------------------------------------------------------------------------

void EditorSys_Init(void)
{
    menubar_init();
}

ProfileMark(pm_update, EditorSys_Update)
void EditorSys_Update(void)
{
    ProfileBegin(pm_update);

    menubar_update();

    ProfileEnd(pm_update);
}

void EditorSys_Shutdown(void)
{
    menubar_shutdown();
}

// ----------------------------------------------------------------------------
