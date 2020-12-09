#pragma once

#include "common/macro.h"
#include "audio/modular/common.hpp"

namespace Modular
{
    class alignas(16) Channel
    {
    private:
        ModuleId m_source;
        i32 m_pin;
        u32 m_ticks[kChannelLen];
        Packet m_packets[kChannelLen];

    public:
        ModuleId GetSource() const { return m_source; }
        u32 GetPin() const { return m_pin; }

        void Init(ModuleId source, i32 pin);
        void Shutdown();

        void Cache(u32 tick, const Packet& src)
        {
            tick = tick >> kPacketShift;
            u32 slot = tick & kChannelMask;
            m_ticks[slot] = tick;
            m_packets[slot] = src;
        }

        bool Read(u32 tick, Packet& dst)
        {
            tick = tick >> kPacketShift;
            u32 slot = tick & kChannelMask;
            if (m_ticks[slot] == tick)
            {
                dst = m_packets[slot];
                return true;
            }
            return false;
        }
    };
};
