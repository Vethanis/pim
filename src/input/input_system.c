#include "input_system.h"
#include "containers/queue.h"
#include <sokol/sokol_app.h>

static queue_t ms_events;

static void Notify(InputChannel channel, int32_t id, int32_t modifiers, float valueX, float valueY);
static void OnKey(const sapp_event* evt, int32_t down);
static void OnMouseButton(const sapp_event* evt, int32_t down);
static void OnMouseMove(const sapp_event* evt);
static void OnMouseScroll(const sapp_event* evt);

// ----------------------------------------------------------------------------

void input_sys_init(void)
{
    queue_create(&ms_events, sizeof(input_event_t), EAlloc_Perm);
}

void input_sys_update(void)
{

}

void input_sys_shutdown(void)
{
    queue_destroy(&ms_events);
}

void input_sys_onevent(const struct sapp_event* evt, int32_t kbCaptured)
{
    if (kbCaptured) { return; }

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

void input_sys_frameend(void)
{
    queue_clear(&ms_events);
}

bool input_get_event(int32_t i, input_event_t* dst)
{
    ASSERT(i >= 0);
    ASSERT(dst);
    uint32_t j = (uint32_t)i;
    if (j < queue_size(&ms_events))
    {
        queue_get(&ms_events, j, dst, sizeof(*dst));
        return true;
    }
    return false;
}

// ----------------------------------------------------------------------------

static void Notify(InputChannel channel, int32_t id, int32_t modifiers, float valueX, float valueY)
{
    input_event_t evt =
    {
        .channel = channel,
        .id = id,
        .modifiers = modifiers,
        .x = valueX,
        .y = valueY,
    };
    queue_push(&ms_events, &evt, sizeof(evt));
}

static void OnKey(const sapp_event* evt, int32_t down)
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

static void OnMouseButton(const sapp_event* evt, int32_t down)
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
    const float width = (float)evt->window_width;
    const float height = (float)evt->window_height;

    float x = (float)evt->mouse_x;
    float y = (float)evt->mouse_y;

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