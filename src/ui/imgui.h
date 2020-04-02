#pragma once

#include <imgui/imgui.h>
#include <stdint.h>
#include "common/sort.h"
#include "containers/array.h"
#include "rendering/render_system.h"

namespace ImGui
{
    inline char BytesFormat(int32_t bytes)
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

    inline float BytesValue(int32_t bytes)
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

    inline void Bytes(int32_t bytes)
    {
        ImGui::Text("%.2f%cB", BytesValue(bytes), BytesFormat(bytes));
    }
    inline void Bytes(cstr label, int32_t bytes)
    {
        ImGui::Text("%s: %.2f%cB", label, BytesValue(bytes), BytesFormat(bytes));
    }

    inline void SetNextWindowScale(float x, float y, ImGuiCond cond = ImGuiCond_Once)
    {
        ImGui::SetNextWindowSize({ screen_width() * x, screen_height() * y }, cond);
    }

    template<typename T>
    struct Table
    {
        using LtFn = bool(*)(int32_t metric, const T* items, int32_t a, int32_t b);

        const char* const* m_titles;
        const float* m_scales;
        int32_t m_numColumns;
        LtFn m_cmp;

        int32_t m_metric;
        bool m_reversed;
        const T* m_items;

        bool operator()(int32_t lhs, int32_t rhs) const
        {
            int32_t a = m_reversed ? rhs : lhs;
            int32_t b = m_reversed ? lhs : rhs;
            return m_cmp(m_metric, m_items, a, b);
        }

        Array<int32_t> Begin(const T* const pItems, int32_t itemCount)
        {
            m_items = pItems;
            const float width = ImGui::GetWindowWidth();
            ImGui::Columns(m_numColumns);
            for (int32_t i = 0; i < m_numColumns; ++i)
            {
                ImGui::SetColumnWidth(i, m_scales[i] * width);
            }

            for (int32_t i = 0; i < m_numColumns; ++i)
            {
                if (ImGui::Button(m_titles[i]))
                {
                    if (m_metric != i)
                    {
                        m_reversed = false;
                        m_metric = i;
                    }
                    else
                    {
                        m_reversed = !m_reversed;
                    }
                }
                ImGui::NextColumn();
            }
            ImGui::Separator();

            Array<int32_t> order = CreateArray<int32_t>(EAlloc_Temp, itemCount);
            order.Resize(itemCount);
            for (int32_t i = 0; i < itemCount; ++i)
            {
                order[i] = i;
            }

            Sort(order.begin(), order.size(), *this);

            return order;
        }

        void End(Array<int32_t>& order)
        {
            m_items = nullptr;
            order.Reset();
            ImGui::Columns();
        }
    };
};
