#pragma once

#include "common/macro.h"
#include "containers/gen_id.hpp"

namespace Modular
{
    static constexpr u32 kPacketShift = 6;
    static constexpr u32 kPacketLen = 1 << kPacketShift;
    static constexpr u32 kPacketMask = kPacketLen - 1u;

    static constexpr u32 kChannelShift = 6;
    static constexpr u32 kChannelLen = 1 << kChannelShift;
    static constexpr u32 kChannelMask = kChannelLen - 1u;

    struct ModuleId
    {
        GenId value;

        i32 GetIndex() const { return value.index; }
        bool operator==(ModuleId other) const
        {
            return value == other.value;
        }
    };

    struct ChannelId
    {
        GenId value;

        i32 GetIndex() const { return value.index; }
        bool operator==(ChannelId other) const
        {
            return value == other.value;
        }
    };

    struct alignas(16) Packet
    {
        float values[kPacketLen];
    };
};
