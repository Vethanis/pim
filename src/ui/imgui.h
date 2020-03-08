#pragma once

#include <imgui/imgui.h>

namespace ImGui
{
    inline char BytesFormat(int bytes)
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

    inline float BytesValue(int bytes)
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

    inline void Bytes(const char* label, int bytes)
    {
        ImGui::Text("%s: %.2f%cB", label, BytesValue(bytes), BytesFormat(bytes));
    }
};
