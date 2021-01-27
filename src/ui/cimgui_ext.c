#include "ui/cimgui_ext.h"
#include <cimgui/cimgui.h>

void igExColumns(i32 count)
{
    igColumns(count, NULL, true);
}

bool igExTableHeader(i32 count, const char* const* titles, i32* pSelection)
{
    ASSERT(titles);
    ASSERT(count >= 1);
    igSeparator();
    igExColumns(count);
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

void igExTableFooter(void)
{
    igSeparator();
    igExColumns(1);
}

bool igExSliderInt(const char* label, i32* x, i32 lo, i32 hi)
{
    return igSliderInt(label, x, lo, hi, "%d", 0x0);
}

bool igExSliderFloat(const char* label, float* x, float lo, float hi)
{
    return igSliderFloat(label, x, lo, hi, "%.3f", 0x0);
}
bool igExSliderFloat2(const char* label, float x[2], float lo, float hi)
{
    return igSliderFloat2(label, x, lo, hi, "%.3f", 0x0);
}
bool igExSliderFloat3(const char* label, float x[3], float lo, float hi)
{
    return igSliderFloat3(label, x, lo, hi, "%.3f", 0x0);
}
bool igExSliderFloat4(const char* label, float x[4], float lo, float hi)
{
    return igSliderFloat4(label, x, lo, hi, "%.3f", 0x0);
}
bool igExCollapsingHeader1(const char* label)
{
    return igCollapsingHeaderTreeNodeFlags(label, 0x0);
}
bool igExCollapsingHeader2(const char* label, bool* pVisible)
{
    return igCollapsingHeaderBoolPtr(label, pVisible, 0x0);
}
void igExSetNextWindowPos(ImVec2 pos, ImGuiCond_ cond)
{
    igSetNextWindowPos(pos, cond, (ImVec2){ 0 });
}
void igExSameLine(void)
{
    igSameLine(0.0f, -1.0f);
}
void igExLogToClipboard(void)
{
    igLogToClipboard(-1);
}
bool igExButton(const char* label)
{
    return igButton(label, (ImVec2) { 0 });
}
