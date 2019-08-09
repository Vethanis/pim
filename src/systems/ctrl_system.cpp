#include "ctrl_system.h"

#include <sokol/sokol_app.h>
#include <imgui/imgui.h>

#include "common/array.h"
#include "common/ring.h"

namespace CtrlSystem
{
    struct EventBuffer
    {
        static constexpr u32 Length = 64u;
        Ring<Length> ring;
        sapp_event_type evtTypes[Length];
        sapp_keycode keycodes[Length];
        u64 frames[Length];
    };

    static EventBuffer ms_buffer;

    void Init()
    {
        ms_buffer.ring.Clear();
    }
    void Update(float dt)
    {
        ImGui::SetNextWindowSize({ 400.0f, 400.0f }, ImGuiCond_Once);
        ImGui::Begin("Control Events");
        {
            ImGui::Columns(3);
            ImGui::Text("Event Type"); ImGui::NextColumn();
            ImGui::Text("Key Code"); ImGui::NextColumn();
            ImGui::Text("Frame"); ImGui::NextColumn();
            ImGui::Separator();

            for (u32 i = ms_buffer.ring.RBegin();
                i != ms_buffer.ring.REnd();
                i = ms_buffer.ring.Prev(i))
            {
                ImGui::Text("%-8d", ms_buffer.evtTypes[i]); ImGui::NextColumn();
                ImGui::Text("%-8d", ms_buffer.keycodes[i]); ImGui::NextColumn();
                ImGui::Text("%-8u", ms_buffer.frames[i]); ImGui::NextColumn();
            }

            ImGui::Columns();
        }
        ImGui::End();
    }
    void Shutdown()
    {

    }
    void OnEvent(const sapp_event* evt, bool keyboardCaptured)
    {
        if (keyboardCaptured) { return; }
        
        u32 i = ms_buffer.ring.Overwrite();
        ms_buffer.evtTypes[i] = evt->type;
        ms_buffer.keycodes[i] = evt->key_code;
        ms_buffer.frames[i] = evt->frame_count;

        if (evt->key_code == SAPP_KEYCODE_ESCAPE)
        {
            sapp_request_quit();
        }
    }
};
