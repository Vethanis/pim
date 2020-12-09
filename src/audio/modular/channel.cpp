#include "audio/modular/channel.hpp"
#include <string.h>

namespace Modular
{

    void Channel::Init(ModuleId source, i32 pin)
    {
        m_source = source;
        m_pin = pin;
        memset(m_ticks, 0, sizeof(m_ticks));
    }

    void Channel::Shutdown()
    {
        m_source = {};
        m_pin = 0;
        memset(m_ticks, 0, sizeof(m_ticks));
    }
};
