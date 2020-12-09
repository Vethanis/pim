#include "audio/modular/board.hpp"
#include "audio/modular/channel.hpp"
#include "audio/modular/module.hpp"

namespace Modular
{
    void Board::Init()
    {

    }

    void Board::Shutdown()
    {
        for (i32 index : m_moduleIds)
        {
            Module* mod = m_modules[index];
            m_modules[index] = NULL;
            ASSERT(mod);
            mod->~Module();
            pim_free(mod);
        }
        m_channelIds.Reset();
        m_channels.Reset();
        m_moduleIds.Reset();
        m_modules.Reset();
    }

    ChannelId Board::AllocChannel(ModuleId source, i32 pin)
    {
        GenId gid = m_channelIds.Alloc();
        i32 slot = gid.index;
        if (slot >= m_channels.Size())
        {
            m_channels.Grow();
        }
        m_channels[slot].Init(source, pin);
        return { gid };
    }

    ModuleId Board::AllocModule(Module* mod)
    {
        ASSERT(mod);
        if (!mod)
        {
            return {};
        }
        GenId gid = m_moduleIds.Alloc();
        i32 slot = gid.index;
        if (slot >= m_modules.Size())
        {
            m_modules.Grow();
        }
        m_modules[slot] = mod;
        return { gid };
    }

    bool Board::Free(ChannelId cid)
    {
        if (m_channelIds.Free(cid.value))
        {
            m_channels[cid.GetIndex()].Shutdown();
            return true;
        }
        return false;
    }

    bool Board::Free(ModuleId id)
    {
        if (m_moduleIds.Free(id.value))
        {
            i32 slot = id.GetIndex();
            Module* mod = m_modules[slot];
            m_modules[slot] = NULL;
            ASSERT(mod);
            for (ChannelId output : mod->GetOutputs())
            {
                Free(output);
            }
            mod->~Module();
            pim_free(mod);
            return true;
        }
        return false;
    }

    bool Board::Read(ChannelId chanId, u32 tick, Packet& dst)
    {
        Channel *const channel = Get(chanId);
        if (channel)
        {
            if (channel->Read(tick, dst))
            {
                return true;
            }
            else
            {
                Module* source = Get(channel->GetSource());
                if (source)
                {
                    source->Sample(channel->GetPin(), tick, dst);
                    channel->Cache(tick, dst);
                    return true;
                }
            }
        }
        return false;
    }

    bool Board::Connect(ModuleId srcId, i32 srcPin, ModuleId dstId, i32 dstPin)
    {
        Module* src = Get(srcId);
        Module* dst = Get(dstId);
        if (src && dst)
        {
            ChannelId channel = src->GetOutput(srcPin);
            if (!Exists(channel))
            {
                channel = AllocChannel(srcId, srcPin);
                src->SetOutput(srcPin, channel);
            }
            dst->SetInput(dstPin, channel);
            return true;
        }
        return false;
    }

    bool Board::Disconnect(ModuleId srcId, i32 srcPin, ModuleId dstId, i32 dstPin)
    {
        Module* dst = Get(dstId);
        if (dst)
        {
            dst->SetInput(dstPin, ChannelId{});
            return true;
        }
        return false;
    }
};
