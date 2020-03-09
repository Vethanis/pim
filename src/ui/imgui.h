#pragma once

#include <imgui/imgui.h>
#include "rendering/screen.h"
#include "common/sort.h"
#include "containers/array.h"

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

    inline f32 BytesValue(i32 bytes)
    {
        if (bytes >= (1 << 30))
        {
            return (f32)bytes / (f32)(1 << 30);
        }
        if (bytes >= (1 << 20))
        {
            return (f32)bytes / (f32)(1 << 20);
        }
        if (bytes >= (1 << 10))
        {
            return (f32)bytes / (f32)(1 << 10);
        }
        return (f32)bytes;
    }

    inline void Bytes(i32 bytes)
    {
        ImGui::Text("%.2f%cB", BytesValue(bytes), BytesFormat(bytes));
    }
    inline void Bytes(cstr label, i32 bytes)
    {
        ImGui::Text("%s: %.2f%cB", label, BytesValue(bytes), BytesFormat(bytes));
    }

    inline void SetNextWindowScale(f32 x, f32 y, ImGuiCond cond = ImGuiCond_Once)
    {
        ImGui::SetNextWindowSize({ Screen::Width() * x, Screen::Height() * y }, cond);
    }

    template<typename T>
    struct Table
    {
        using LtFn = bool(*)(i32 metric, const T* items, i32 a, i32 b);

        const char* const* m_titles;
        const f32* m_scales;
        i32 m_numColumns;
        LtFn m_cmp;

        i32 m_metric;
        bool m_reversed;
        const T* m_items;

        bool operator()(i32 lhs, i32 rhs) const
        {
            i32 a = m_reversed ? rhs : lhs;
            i32 b = m_reversed ? lhs : rhs;
            return m_cmp(m_metric, m_items, a, b);
        }

        Array<i32> Begin(const T* const pItems, i32 itemCount)
        {
            m_items = pItems;
            const f32 width = ImGui::GetWindowWidth();
            ImGui::Columns(m_numColumns);
            for (i32 i = 0; i < m_numColumns; ++i)
            {
                ImGui::SetColumnWidth(i, m_scales[i] * width);
            }

            for (i32 i = 0; i < m_numColumns; ++i)
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

            Array<i32> order = CreateArray<i32>(Alloc_Temp, itemCount);
            order.Resize(itemCount);
            for (i32 i = 0; i < itemCount; ++i)
            {
                order[i] = i;
            }

            Sort(order.begin(), order.size(), *this);

            return order;
        }

        void End(Array<i32>& order)
        {
            m_items = nullptr;
            order.Reset();
            ImGui::Columns();
        }
    };
};
