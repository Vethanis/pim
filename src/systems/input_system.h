
#include "common/array.h"
#include "common/vec_types.h"

struct sapp_event;

namespace InputSystem
{
    struct KeyEvent
    {
        u64 tick;
        u8 down;
    };

    struct MouseButtonEvent
    {
        u64 tick;
        u8 down;
    };

    struct MousePositionEvent
    {
        u64 tick;
        float2 position;
    };

    struct MouseScrollEvent
    {
        u64 tick;
        float2 position;
    };

    void AddListener(Array<KeyEvent>& dst, u16 key);
    void RemoveListener(Array<KeyEvent>& dst);

    void AddListener(Array<MouseButtonEvent>& dst, u16 button);
    void RemoveListener(Array<MouseButtonEvent>& dst);

    void AddListener(Array<MousePositionEvent>& dst);
    void RemoveListener(Array<MousePositionEvent>& dst);

    void AddListener(Array<MouseScrollEvent>& dst);
    void RemoveListener(Array<MouseScrollEvent>& dst);

    void Init();
    void Update();
    void Shutdown();
    void OnEvent(const sapp_event* evt, bool keyboardCaptured);
    void Visualize();
};
