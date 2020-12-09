#pragma once

#include "audio/modular/module.hpp"
#include "audio/modular/channel.hpp"

namespace Modular
{
    class MidiModule final : public Module
    {
    public:
        enum Pins
        {
            Pin_Frequency,
            Pin_Gate,
            Pin_COUNT
        };
        void OnEvent(u32 tick);
        void Sample(i32 pin, u32 tick, Packet& result) override;
    private:
        Channel m_channels[Pin_COUNT];
    };
};
