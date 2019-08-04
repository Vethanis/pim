#include "ctrl_system.h"

#include <sokol/sokol_app.h>
#include <imgui/imgui.h>

#include "common/array.h"

namespace CtrlSystem
{
    static Array<sapp_event_type> ms_evtTypes;
    static Array<sapp_keycode> ms_keycodes;
    static Array<u64> ms_frames;

    void Init()
    {
        ms_evtTypes.init();
        ms_keycodes.init();
        ms_frames.init();
    }
    void Update(float dt)
    {
        const i32 maxEvents = 64;
        ms_evtTypes.shiftTail(maxEvents);
        ms_keycodes.shiftTail(maxEvents);
        ms_frames.shiftTail(maxEvents);

        ImGui::Begin("Control Events");
        {
            ImGui::Columns(3);
            ImGui::Text("Event Type"); ImGui::NextColumn();
            ImGui::Text("Key Code"); ImGui::NextColumn();
            ImGui::Text("Frame"); ImGui::NextColumn();
            ImGui::Separator();

            const i32 len = ms_evtTypes.size();
            for (i32 i = len - 1; i >= 0; --i)
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
        ms_evtTypes.reset();
        ms_keycodes.reset();
        ms_frames.reset();
    }
    void OnEvent(const sapp_event* evt, bool keyboardCaptured)
    {
        if (keyboardCaptured) { return; }

        ms_evtTypes.grow() = evt->type;
        ms_keycodes.grow() = evt->key_code;
        ms_frames.grow() = evt->frame_count;

        if (evt->key_code == SAPP_KEYCODE_ESCAPE)
        {
            sapp_request_quit();
        }
    }
};
