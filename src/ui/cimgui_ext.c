#include "ui/cimgui_ext.h"
#include "ui/cimgui.h"

bool igTableHeader(i32 count, const char** titles, i32* pSelection)
{
    ASSERT(titles);
    ASSERT(count >= 1);
    igSeparator();
    igColumns(count);
    bool clicked = false;
    for (i32 i = 0; i < count; ++i)
    {
        if (igSmallButton(titles[i]))
        {
            if (pSelection)
            {
                *pSelection = i;
            }
            clicked = true;
        }
        igNextColumn();
    }
    igSeparator();
    return clicked;
}

void igTableFooter(void)
{
    igSeparator();
    igColumns(1);
}
