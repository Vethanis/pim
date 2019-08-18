#include "input_system.h"

#include <sokol/sokol_app.h>
#include <inttypes.h>
#include <imgui/imgui.h>

#include "common/array.h"
#include "systems/time_system.h"

#include "common/reflection.h"

namespace InputSystem
{
    struct KeyListener
    {
        Array<KeyEvent>* dst;
        u16 key;
    };

    struct MouseButtonListener
    {
        Array<MouseButtonEvent>* dst;
        u16 button;
    };

    struct MousePositionListener
    {
        Array<MousePositionEvent>* dst;
    };

    struct MouseScrollListener
    {
        Array<MouseScrollEvent>* dst;
    };

    static Array<KeyListener> ms_keyListeners;
    static Array<MouseButtonListener> ms_mouseButtonListeners;
    static Array<MousePositionListener> ms_mousePositionListeners;
    static Array<MouseScrollListener> ms_mouseScrollListeners;

    static void OnKey(const sapp_event* evt)
    {
        if (evt->key_repeat)
        {
            return;
        }

        const u64 tick = TimeSystem::Now();
        const u16 key = evt->key_code;
        const u8 down = evt->type == SAPP_EVENTTYPE_KEY_DOWN;

        for (KeyListener& listener : ms_keyListeners)
        {
            if (listener.key == key)
            {
                listener.dst->grow() = { tick, down };
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
        const u64 tick = TimeSystem::Now();
        const u16 button = evt->mouse_button;
        const u8 down = evt->type == SAPP_EVENTTYPE_MOUSE_DOWN;
        for (MouseButtonListener& listener : ms_mouseButtonListeners)
        {
            if (listener.button == button)
            {
                listener.dst->grow() = { tick, down };
            }
        }
    }
    static void OnMouseMove(const sapp_event* evt)
    {
        const u64 tick = TimeSystem::Now();
        const float2 position = { evt->mouse_x, evt->mouse_y };
        for (MousePositionListener& listener : ms_mousePositionListeners)
        {
            listener.dst->grow() = { tick, position };
        }
    }
    static void OnMouseScroll(const sapp_event* evt)
    {
        const u64 tick = TimeSystem::Now();
        const float2 position = { evt->scroll_x, evt->scroll_y };
        for (MouseScrollListener& listener : ms_mouseScrollListeners)
        {
            listener.dst->grow() = { tick, position };
        }
    }

    void AddListener(Array<KeyEvent>& dst, u16 key)
    {
        ms_keyListeners.grow() = { &dst, key };
    }
    void RemoveListener(Array<KeyEvent>& dst, u16 key)
    {
        ms_keyListeners.findRemove({ &dst, key });
    }

    void AddListener(Array<MouseButtonEvent>& dst, u16 button)
    {
        ms_mouseButtonListeners.grow() = { &dst, button };
    }
    void RemoveListener(Array<MouseButtonEvent>& dst, u16 button)
    {
        ms_mouseButtonListeners.findRemove({ &dst, button });
    }

    void AddListener(Array<MousePositionEvent>& dst)
    {
        ms_mousePositionListeners.grow() = { &dst };
    }
    void RemoveListener(Array<MousePositionEvent>& dst)
    {
        ms_mousePositionListeners.findRemove({ &dst });
    }

    void AddListener(Array<MouseScrollEvent>& dst)
    {
        ms_mouseScrollListeners.grow() = { &dst };
    }
    void RemoveListener(Array<MouseScrollEvent>& dst)
    {
        ms_mouseScrollListeners.findRemove({ &dst });
    }

    void Init()
    {

    }
    void Update()
    {

    }
    void Shutdown()
    {

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
