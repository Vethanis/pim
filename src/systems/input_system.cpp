#include "input_system.h"

#include <sokol/sokol_app.h>
#include <inttypes.h>
#include <imgui/imgui.h>

#include "containers/array.h"
#include "containers/dict.h"
#include "systems/time_system.h"

#include "common/reflection.h"

namespace InputSystem
{
    static Dict<u16, ButtonChannel*> ms_buttonChannels;
    static Dict<u16, AxisChannel*> ms_axisChannels;
    static Dict<u32, u16> ms_buttonBinds;
    static Dict<u32, u16> ms_axisBinds;

    static u32 ToDictKey(u16 device, u16 id)
    {
        u32 y = device;
        y <<= 16;
        y |= id;
        return y;
    }

    static void OnKey(const sapp_event* evt)
    {
        if (evt->key_repeat)
        {
            return;
        }

        const u16* pId = ms_buttonBinds.get(ToDictKey(BD_Key, evt->key_code));
        if (pId)
        {
            ButtonChannel** ppChannel = ms_buttonChannels.get(*pId);
            if (ppChannel && *ppChannel)
            {
                ButtonChannel* pChannel = *ppChannel;
                u32 i = pChannel->ring.Overwrite();
                pChannel->ticks[i] = TimeSystem::Now();
                pChannel->down[i] = evt->type == SAPP_EVENTTYPE_KEY_DOWN;
            }
        }

        if (evt->key_code == SAPP_KEYCODE_ESCAPE)
        {
            sapp_request_quit();
        }
    }
    static void OnMouseButton(const sapp_event* evt)
    {
        if (evt->key_repeat)
        {
            return;
        }
        const u16* pId = ms_buttonBinds.get(ToDictKey(BD_Mouse, evt->mouse_button));
        if (pId)
        {
            ButtonChannel** ppChannel = ms_buttonChannels.get(*pId);
            if (ppChannel && *ppChannel)
            {
                ButtonChannel* pChannel = *ppChannel;
                u32 i = pChannel->ring.Overwrite();
                pChannel->ticks[i] = TimeSystem::Now();
                pChannel->down[i] = evt->type == SAPP_EVENTTYPE_MOUSE_DOWN;
            }
        }
    }
    static void OnMouseMove(const sapp_event* evt)
    {
        const u16* pId = ms_axisBinds.get(ToDictKey(AD_Mouse, 0));
        if (pId)
        {
            AxisChannel** ppChannel = ms_axisChannels.get(*pId);
            if (ppChannel && *ppChannel)
            {
                AxisChannel* pChannel = *ppChannel;
                u32 i = pChannel->ring.Overwrite();
                pChannel->positions[i] = { evt->mouse_x, evt->mouse_y };
                pChannel->ticks[i] = TimeSystem::Now();
            }
        }
    }
    static void OnMouseScroll(const sapp_event* evt)
    {
        const u16* pId = ms_axisBinds.get(ToDictKey(AD_Mouse, 1));
        if (pId)
        {
            AxisChannel** ppChannel = ms_axisChannels.get(*pId);
            if (ppChannel && *ppChannel)
            {
                AxisChannel* pChannel = *ppChannel;
                u32 i = pChannel->ring.Overwrite();
                pChannel->positions[i] = { evt->scroll_x, evt->scroll_y };
                pChannel->ticks[i] = TimeSystem::Now();
            }
        }
    }

    void Bind(ButtonDevice dev, u16 button, u16 channel)
    {
        ms_buttonBinds[ToDictKey(dev, button)] = channel;
    }
    void Unbind(ButtonDevice dev, u16 button)
    {
        ms_buttonBinds.remove(ToDictKey(dev, button));
    }
    void Bind(AxisDevice dev, u16 axis, u16 channel)
    {
        ms_axisBinds[ToDictKey(dev, axis)] = channel;
    }
    void Unbind(AxisDevice dev, u16 axis)
    {
        ms_axisBinds.remove(ToDictKey(dev, axis));
    }

    void Register(u16 id, ButtonChannel& ch)
    {
        ms_buttonChannels[id] = &ch;
    }
    void UnregisterButton(u16 id)
    {
        ms_buttonChannels.remove(id);
    }
    void Register(u16 id, AxisChannel& ch)
    {
        ms_axisChannels[id] = &ch;
    }
    void UnregisterAxis(u16 id)
    {
        ms_axisChannels.remove(id);
    }

    void Init()
    {
        ms_buttonChannels.init(Allocator_Malloc);
        ms_axisChannels.init(Allocator_Malloc);
        ms_buttonBinds.init(Allocator_Malloc);
        ms_axisBinds.init(Allocator_Malloc);
    }
    void Update()
    {

    }
    void Shutdown()
    {
        ms_buttonChannels.reset();
        ms_axisChannels.reset();
        ms_buttonBinds.reset();
        ms_axisBinds.reset();
    }
    void OnEvent(const sapp_event* evt, bool keyboardCaptured)
    {
        if (keyboardCaptured) { return; }

        switch (evt->type)
        {
        case SAPP_EVENTTYPE_KEY_DOWN:
        case SAPP_EVENTTYPE_KEY_UP:
            OnKey(evt);
            break;
        case SAPP_EVENTTYPE_MOUSE_DOWN:
        case SAPP_EVENTTYPE_MOUSE_UP:
            OnMouseButton(evt);
            break;
        case SAPP_EVENTTYPE_MOUSE_MOVE:
            OnMouseMove(evt);
            break;
        case SAPP_EVENTTYPE_MOUSE_SCROLL:
            OnMouseScroll(evt);
            break;
        default:
            break;
        }
    }
    void Visualize()
    {
        ImGui::SetNextWindowSize({ 400.0f, 400.0f }, ImGuiCond_Once);
        ImGui::Begin("CtrlSystem");
        {
            if (ImGui::CollapsingHeader("Keys"))
            {
                ImGui::Columns(3);
                ImGui::Text("Tick"); ImGui::NextColumn();
                ImGui::Text("Key Code"); ImGui::NextColumn();
                ImGui::Text("State"); ImGui::NextColumn();
                ImGui::Separator();

                ImGui::Columns();
            }

            const RfType* info = &RfGet<RfTest*>();
            ImGui::Text("%s %u %u", info->name, info->size, info->align);
            if (info->deref)
            {
                info = info->deref;
                ImGui::Text("%s %u %u", info->name, info->size, info->align);
            }
            for (const RfMember& member : info->members)
            {
                ImGui::Text("%s::%s %s at %u", info->name, member.name, member.value->name, member.offset);
            }
        }
        ImGui::End();
    }
};
