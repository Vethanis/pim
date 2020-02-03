#include "input_system.h"

#include <sokol/sokol_app.h>

#include "common/macro.h"
#include "containers/array.h"
#include "common/find.h"
#include "components/system.h"
#include "common/comparator_util.h"

// ----------------------------------------------------------------------------

static Array<InputListener> ms_listeners[InputChannel_COUNT];

// ----------------------------------------------------------------------------

static void Notify(InputChannel channel, u16 id, u8 modifiers, f32 valueX, f32 valueY)
{
    InputEvent evt = {};
    evt.channel = channel;
    evt.id = id;
    evt.modifiers = modifiers;
    for (InputListener listener : ms_listeners[channel])
    {
        listener(evt, valueX, valueY);
    }
}

static void OnKey(const sapp_event* evt, bool down)
{
    if (evt->key_repeat)
    {
        return;
    }
    Notify(
        InputChannel_Keyboard,
        evt->key_code,
        evt->modifiers,
        down ? 1.0f : 0.0f,
        0.0f);
}

static void OnMouseButton(const sapp_event* evt, bool down)
{
    if (evt->key_repeat)
    {
        return;
    }
    Notify(
        InputChannel_MouseButton,
        evt->mouse_button,
        evt->modifiers,
        down ? 1.0f : 0.0f,
        0.0f);
}

static void OnMouseMove(const sapp_event* evt)
{
    const f32 width = (f32)evt->window_width;
    const f32 height = (f32)evt->window_height;

    f32 x = (f32)evt->mouse_x;
    f32 y = (f32)evt->mouse_y;

    // convert pixel to snorm
    x = 2.0f * (x / width) - 1.0f;
    y = 2.0f * (y / height) - 1.0f;

    // flip y so that +y == up, as it is more intuitive.
    // +y == down is a legacy artifact.
    y = -y;

    Notify(InputChannel_MouseMove, 0, evt->modifiers, x, y);
}

static void OnMouseScroll(const sapp_event* evt)
{
    Notify(
        InputChannel_MouseScroll,
        0,
        evt->modifiers,
        evt->scroll_x,
        evt->scroll_y);
}

// ----------------------------------------------------------------------------

namespace InputSystem
{
    static constexpr auto ListenerEq = PtrEquatable<InputListener>();

    bool Register(InputChannel channel, InputListener listener)
    {
        ASSERT(listener);
        return FindAdd(ms_listeners[channel], listener, ListenerEq);
    }

    bool Remove(InputChannel channel, InputListener listener)
    {
        ASSERT(listener);
        return FindRemove(ms_listeners[channel], listener, ListenerEq);
    }

    void OnEvent(const sapp_event* evt, bool keyboardCaptured)
    {
        if (keyboardCaptured) { return; }

        switch (evt->type)
        {
        case SAPP_EVENTTYPE_KEY_DOWN:
            OnKey(evt, true); break;
        case SAPP_EVENTTYPE_KEY_UP:
            OnKey(evt, false); break;

        case SAPP_EVENTTYPE_MOUSE_DOWN:
            OnMouseButton(evt, true); break;
        case SAPP_EVENTTYPE_MOUSE_UP:
            OnMouseButton(evt, false); break;

        case SAPP_EVENTTYPE_MOUSE_MOVE:
            OnMouseMove(evt); break;

        case SAPP_EVENTTYPE_MOUSE_SCROLL:
            OnMouseScroll(evt); break;

        default: break;
        }
    }

    // ------------------------------------------------------------------------

    static void Init()
    {
        for (Array<InputListener>& listeners : ms_listeners)
        {
            listeners.Init(Alloc_Pool);
        }
    }

    static void Update()
    {

    }

    static void Shutdown()
    {
        for (Array<InputListener>& listeners : ms_listeners)
        {
            listeners.Reset();
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
