#include "input_system.h"

#include <sokol/sokol_app.h>
#include "ui/imgui.h"

#include "common/macro.h"
#include "containers/array.h"
#include "containers/dict.h"
#include "systems/time_system.h"

// ----------------------------------------------------------------------------

using ListenerKey = u16;

struct ListenerValue
{
    InputListener listener;
    f32 value;
};

using ListenerStore = Dict<ListenerKey, ListenerValue>;

static ListenerStore ms_stores[InputChannel_Count];
static constexpr cstrc ms_storeNames[] =
{
    "Keyboard",
    "MouseButton",
    "MouseAxis",
};

// ----------------------------------------------------------------------------

static ListenerKey EncodeKey(u16 id, u8 mods)
{
    return (id << 4u) | (mods & InputMod_Mask);
}

static void DecodeKey(ListenerKey key, u16& id, u8& mods)
{
    id = key >> 4u;
    mods = key & InputMod_Mask;
}

static void Notify(InputChannel channel, u16 id, u8 mods, f32 value)
{
    ListenerValue* pValue = ms_stores[channel].get(EncodeKey(id, mods));
    if (pValue)
    {
        pValue->value = value;
        pValue->listener(TimeSystem::Now(), value);
    }
}

static void OnKey(const sapp_event* evt)
{
    if (evt->key_repeat)
    {
        return;
    }
    f32 value = (evt->type == SAPP_EVENTTYPE_KEY_DOWN) ? 1.0f : 0.0f;
    Notify(InputChannel_Keyboard, evt->key_code, evt->modifiers, value);
}

static void OnMouseButton(const sapp_event* evt)
{
    if (evt->key_repeat)
    {
        return;
    }
    f32 value = (evt->type == SAPP_EVENTTYPE_MOUSE_DOWN) ? 1.0f : 0.0f;
    Notify(InputChannel_MouseButton, evt->mouse_button, evt->modifiers, value);
}

static void OnMouseMove(const sapp_event* evt)
{
    Notify(InputChannel_MouseAxis, MouseAxis_X, evt->modifiers, evt->mouse_x);
    Notify(InputChannel_MouseAxis, MouseAxis_Y, evt->modifiers, evt->mouse_y);
}

static void OnMouseScroll(const sapp_event* evt)
{
    Notify(InputChannel_MouseAxis, MouseAxis_Z, evt->modifiers, evt->scroll_x);
    Notify(InputChannel_MouseAxis, MouseAxis_W, evt->modifiers, evt->scroll_y);
}

// ----------------------------------------------------------------------------

namespace InputSystem
{
    void Listen(InputListenerDesc desc)
    {
        u32 id = EncodeKey(desc.id, desc.modifiers);
        if (desc.listener)
        {
            ms_stores[desc.channel][id] = { desc.listener, 0.0f };
        }
        else
        {
            ms_stores[desc.channel].remove(id);
        }
    }

    void Init()
    {
        for (auto& store : ms_stores)
        {
            store.init();
        }
    }

    void Update()
    {

    }

    void Shutdown()
    {
        for (auto& store : ms_stores)
        {
            store.reset();
        }
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
            for (u32 i = 0; i < CountOf(ms_stores); ++i)
            {
                if (ImGui::CollapsingHeader(ms_storeNames[i]))
                {
                    const usize numListeners = ms_stores[i].size();
                    const ListenerKey* keys = ms_stores[i].m_keys.begin();
                    const ListenerValue* values = ms_stores[i].m_values.begin();

                    ImGui::Columns(4, "keylisteners");
                    ImGui::Text("Id"); ImGui::NextColumn();
                    ImGui::Text("Modifiers"); ImGui::NextColumn();
                    ImGui::Text("Listener"); ImGui::NextColumn();
                    ImGui::Text("Value"); ImGui::NextColumn();
                    ImGui::Separator();

                    for (usize j = 0; j < numListeners; ++j)
                    {
                        u16 id;
                        u8 mods;
                        DecodeKey(keys[j], id, mods);
                        ImGui::Text("%u", id); ImGui::NextColumn();
                        ImGui::Text("%x", mods); ImGui::NextColumn();
                        ImGui::Text("%p", values[j].listener); ImGui::NextColumn();
                        ImGui::Text("%g", values[j].value); ImGui::NextColumn();
                    }

                    ImGui::Columns();
                }
            }
        }
        ImGui::End();
    }

    System GetSystem()
    {
        System sys;
        sys.Init = Init;
        sys.Update = Update;
        sys.Shutdown = Shutdown;
        sys.Visualize = Visualize;
        sys.enabled = true;
        sys.visualizing = false;
        return sys;
    }
};
