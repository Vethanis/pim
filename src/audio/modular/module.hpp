#pragma once

#include "common/macro.h"
#include "audio/modular/common.hpp"
#include "containers/array.hpp"

namespace Modular
{
    class Module
    {
    private:
        ModuleId m_id;
        Array<ChannelId> m_inputs;
        Array<ChannelId> m_outputs;
    public:
        virtual ~Module() {}
        virtual void Sample(i32 pin, u32 tick, Packet& result) = 0;

        ModuleId GetId() const { return m_id; }
        void SetId(ModuleId id) { m_id = id; }
        const Array<ChannelId>& GetInputs() const { return m_inputs; }
        const Array<ChannelId>& GetOutputs() const { return m_outputs; }
        ChannelId GetInput(i32 pin) const
        {
            return m_inputs.InRange(pin) ? m_inputs[pin] : ChannelId{};
        }
        ChannelId GetOutput(i32 pin) const
        {
            return m_outputs.InRange(pin) ? m_outputs[pin] : ChannelId{};
        }
        void SetInput(i32 pin, ChannelId id)
        {
            m_inputs.EnsureSize(pin + 1);
            m_inputs[pin] = id;
        }
        void SetOutput(i32 pin, ChannelId id)
        {
            m_outputs.EnsureSize(pin + 1);
            m_outputs[pin] = id;
        }
    };
};
