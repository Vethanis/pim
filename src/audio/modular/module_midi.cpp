#include "audio/modular/module_midi.hpp"
#include "input/input_system.h"

namespace Modular
{
    MidiModule::MidiModule() : Module()
    {
        m_port = 0;
        m_hdl = {};
    }

    MidiModule::~MidiModule()
    {
        midi_close(m_hdl);
        m_hdl = {};
    }

    bool MidiModule::SetPort(i32 port)
    {
        midi_close(m_hdl);
        m_hdl = midi_open(port, MidiCallback, this);
        return midi_exists(m_hdl);
    }

    bool MidiModule::IsConnected() const
    {
        return midi_exists(m_hdl);
    }

    void MidiModule::MidiCallback(const midimsg_t* msg, void* usr)
    {
        ASSERT(msg);
        ASSERT(usr);
        reinterpret_cast<MidiModule*>(usr)->OnMidi(*msg);
    }

    void MidiModule::OnMidi(midimsg_t msg)
    {

    }

    void MidiModule::Sample(i32 pin, u32 tick, Packet& result)
    {
        ASSERT(pin >= 0);
        ASSERT(pin < Pin_COUNT);
        m_channels[pin].Read(tick, result);
    }

    void MidiModule::OnGui()
    {

    }
};
