#pragma once

#include "audio/modular/module.hpp"
#include "audio/modular/channel.hpp"
#include "audio/midi_system.h"

typedef struct midimsg_s midimsg_t;

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

        MidiModule();
        ~MidiModule();

        i32 GetPort() const { return m_port; }
        bool SetPort(i32 port);
        bool IsConnected() const;

        void Sample(i32 pin, u32 tick, Packet& result) final;
        void OnGui() final;

        static void MidiCallback(const midimsg_t* msg, void* usr);
    private:
        void OnMidi(midimsg_t msg);

        i32 m_port;
        midihdl_t m_hdl;
        Channel m_channels[Pin_COUNT];
    };
};
