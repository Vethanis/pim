#pragma once

#include "common/macro.h"
#include "audio/modular/common.hpp"
#include "audio/modular/channel.hpp"
#include "containers/id_alloc.hpp"
#include "containers/array.hpp"

namespace Modular
{
    class Module;

    class Board
    {
    private:
        IdAllocator m_channelIds;
        Array<Channel> m_channels;

        IdAllocator m_moduleIds;
        Array<Module*> m_modules;

    public:
        void Init();
        void Shutdown();

        bool Exists(ChannelId id) const
        {
            return m_channelIds.Exists(id.value);
        }
        bool Exists(ModuleId id) const
        {
            return m_moduleIds.Exists(id.value);
        }

        Channel* Get(ChannelId id)
        {
            return Exists(id) ? &m_channels[id.GetIndex()] : NULL;
        }
        Module* Get(ModuleId id)
        {
            return Exists(id) ? m_modules[id.GetIndex()] : NULL;
        }

        ChannelId AllocChannel(ModuleId source, i32 pin);
        ModuleId AllocModule(Module* mod);

        bool Free(ChannelId cid);
        bool Free(ModuleId id);

        bool Read(ChannelId chanId, u32 tick, Packet& dst);

        bool Connect(ModuleId srcId, i32 srcPin, ModuleId dstId, i32 dstPin);
        bool Disconnect(ModuleId srcId, i32 srcPin, ModuleId dstId, i32 dstPin);
    };
};
