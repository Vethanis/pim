#include "ctrl_system.h"

#include <sokol/sokol_app.h>
#include <imgui/imgui.h>

#include "common/array.h"
#include "common/ringbuf.h"

namespace CtrlSystem
{
    constexpr u32 BufferLength = 64u;
    static RingBuf<sapp_event_type, BufferLength> ms_evtTypes;
    static RingBuf<sapp_keycode, BufferLength> ms_keycodes;
    static RingBuf<u64, BufferLength> ms_frames;

    void Init()
    {
        ms_evtTypes.Reset();
        ms_keycodes.Reset();
        ms_frames.Reset();
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

            for (u32 i = ms_evtTypes.RBegin();
                i != ms_evtTypes.REnd();
                i = ms_evtTypes.Prev(i))
            {
                ImGui::Text("%-8d", ms_evtTypes[i]); ImGui::NextColumn();
                ImGui::Text("%-8d", ms_keycodes[i]); ImGui::NextColumn();
                ImGui::Text("%-8u", ms_frames[i]); ImGui::NextColumn();
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

        ms_evtTypes.Overwrite(evt->type);
        ms_keycodes.Overwrite(evt->key_code);
        ms_frames.Overwrite(evt->frame_count);

        if (evt->key_code == SAPP_KEYCODE_ESCAPE)
        {
            sapp_request_quit();
        }
    }
};
