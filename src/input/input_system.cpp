#include "input_system.h"

#include <sokol/sokol_app.h>
#include "ui/imgui.h"

#include "common/macro.h"
#include "containers/array.h"
#include "common/find.h"
#include "time/time_system.h"
#include "components/system.h"

// ----------------------------------------------------------------------------

static Array<InputListener> ms_listeners[InputChannel_Count];
static constexpr cstrc ms_storeNames[] =
{
    "Keyboard",
    "MouseButton",
    "MouseAxis",
};

// ----------------------------------------------------------------------------

static void Notify(InputChannel channel, u16 id, u8 mods, f32 value)
{
    InputEvent evt;
    evt.channel = channel;
    evt.id = id;
    evt.mods = mods;
    evt.value = value;
    evt.tick = TimeSystem::Now();
    for (InputListener onEvent : ms_listeners[channel])
    {
        onEvent(evt);
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
    void Listen(InputChannel channel, InputListener onEvent)
    {
        ASSERT(onEvent);
        if (onEvent)
        {
            UniquePush(ms_listeners[channel], onEvent);
        }
    }

    void Deafen(InputChannel channel, InputListener onEvent)
    {
        ASSERT(onEvent);
        if (onEvent)
        {
            FindRemove(ms_listeners[channel], onEvent);
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

    // ------------------------------------------------------------------------

    static void Init()
    {
        for (auto& store : ms_listeners)
        {
            store.Init(Alloc_Pool);
        }
    }

    static void Update()
    {

    }

    static void Shutdown()
    {
        for (auto& store : ms_listeners)
        {
            store.Reset();
        }
    }

    static constexpr System ms_system =
    {
        ToGuid("InputSystem"),
        { 0, 0 },
        Init,
        Update,
        Shutdown,
    };
    static RegisterSystem ms_register(ms_system);
};
