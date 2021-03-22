#include "editor/editor.h"
#include "editor/menubar.h"
#include "common/profiler.h"

// ----------------------------------------------------------------------------

void EditorSys_Init(void)
{
    MenuBar_Init();
}

ProfileMark(pm_update, EditorSys_Update)
void EditorSys_Update(void)
{
    ProfileBegin(pm_update);

    MenuBar_Update();

    ProfileEnd(pm_update);
}

void EditorSys_Shutdown(void)
{
    MenuBar_Shutdown();
}

// ----------------------------------------------------------------------------
