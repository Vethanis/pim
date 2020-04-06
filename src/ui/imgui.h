#pragma once

#include <imgui/imgui.h>
#include "rendering/render_system.h"

namespace ImGui
{
    inline char BytesFormat(i32 bytes)
    {
        if (bytes >= (1 << 30))
        {
            return 'G';
        }
        if (bytes >= (1 << 20))
        {
            return 'M';
        }
        if (bytes >= (1 << 10))
        {
            return 'K';
        }
        return ' ';
    }

    inline float BytesValue(i32 bytes)
    {
        if (bytes >= (1 << 30))
        {
            return (float)bytes / (float)(1 << 30);
        }
        if (bytes >= (1 << 20))
        {
            return (float)bytes / (float)(1 << 20);
        }
        if (bytes >= (1 << 10))
        {
            return (float)bytes / (float)(1 << 10);
        }
        return (float)bytes;
    }

    inline void Bytes(i32 bytes)
    {
        ImGui::Text("%.2f%cB", BytesValue(bytes), BytesFormat(bytes));
    }
    inline void Bytes(const char* label, i32 bytes)
    {
        ImGui::Text("%s: %.2f%cB", label, BytesValue(bytes), BytesFormat(bytes));
    }

    inline void SetNextWindowScale(float x, float y, ImGuiCond cond = ImGuiCond_Once)
    {
        ImGui::SetNextWindowSize({ screen_width() * x, screen_height() * y }, cond);
    }
};
