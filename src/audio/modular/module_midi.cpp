#include "audio/modular/module_midi.hpp"
#include "input/input_system.h"

namespace Modular
{
    void MidiModule::Sample(i32 pin, u32 tick, Packet& result)
    {
        ASSERT(pin >= 0);
        ASSERT(pin < Pin_COUNT);
        m_channels[pin].Read(tick, result);
    }
};
