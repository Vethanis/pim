
#include "containers/ring.h"
#include "common/vec_types.h"

struct sapp_event;

namespace InputSystem
{
    enum ButtonDevice : u8
    {
        BD_Key = 0,
        BD_Mouse,
        BD_Pad0,
        BD_Pad1,
        BD_Pad2,
        BD_Pad3,
        BD_Count
    };

    enum AxisDevice : u8
    {
        AD_Mouse = 0,
        AD_Pad0,
        AD_Pad1,
        AD_Pad2,
        AD_Pad3,
        AD_Count
    };

    struct ButtonChannel
    {
        static constexpr u32 Capacity = 16;
        Ring<Capacity> ring;
        u64 ticks[Capacity];
        u8 down[Capacity];
    };

    struct AxisChannel
    {
        static constexpr u32 Capacity = 64;
        Ring<Capacity> ring;
        u64 ticks[Capacity];
        float2 positions[Capacity];
    };

    void Bind(ButtonDevice dev, u16 button, u16 channel);
    void Unbind(ButtonDevice dev, u16 button);
    void Bind(AxisDevice dev, u16 axis, u16 channel);
    void Unbind(AxisDevice dev, u16 axis);

    void Register(u16 channel, ButtonChannel& dst);
    void UnregisterButton(u16 channel);
    void Register(u16 channel, AxisChannel& dst);
    void UnregisterAxis(u16 channel);

    void Init();
    void Update();
    void Shutdown();
    void OnEvent(const sapp_event* evt, bool keyboardCaptured);
    void Visualize();
};
