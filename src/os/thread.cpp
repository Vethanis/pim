#include "os/thread.h"

#include "common/macro.h"
#include "common/minmax.h"
#include "containers/bitpack.h"

static constexpr u64 SpinMultiplier = 100;
static constexpr u64 MaxSpins = 10;

namespace OS
{
    bool SpinLock::TryLock()
    {
        i32 oldCount = m_count.Load(MO_Relaxed);
        return (!oldCount) &&
            m_count.CmpExStrong(oldCount, oldCount + 1, MO_Acquire);
    }

    void SpinLock::Lock()
    {
        if (!TryLock())
        {
            u64 spins = 0;
            i32 oldCount;
            while (true)
            {
                oldCount = m_count.Load(MO_Relaxed);
                if ((!oldCount) &&
                    m_count.CmpExStrong(oldCount, oldCount + 1, MO_AcqRel))
                {
                    return;
                }
                ASSERT(oldCount > 0);
                AtomicSignalFence(MO_Acquire);
                ++spins;
                SpinWait(spins * SpinMultiplier);
            }
        }
    }

    void SpinLock::Unlock()
    {
        i32 oldCount = m_count.Sub(1, MO_Release);
        ASSERT(oldCount > 0);
    }

    // ------------------------------------------------------------------------

    bool LightSema::TryWait()
    {
        i32 oldCount = m_count.Load(MO_Relaxed);
        return (oldCount > 0) &&
            m_count.CmpExStrong(oldCount, oldCount - 1, MO_Acquire);
    }

    void LightSema::Wait()
    {
        if (!TryWait())
        {
            u64 spins = 0;
            i32 oldCount;
            while (spins < MaxSpins)
            {
                oldCount = m_count.Load(MO_Relaxed);
                if ((oldCount > 0) &&
                    m_count.CmpExStrong(oldCount, oldCount - 1, MO_AcqRel))
                {
                    return;
                }
                AtomicSignalFence(MO_Acquire);
                ++spins;
                SpinWait(spins * SpinMultiplier);
            }
            oldCount = m_count.Dec(MO_Acquire);
            if (oldCount <= 0)
            {
                m_sema.Wait();
            }
        }
    }

    void LightSema::Signal(i32 count)
    {
        i32 oldCount = m_count.Add(count, MO_Release);
        i32 toRelease = (-oldCount < count) ? -oldCount : count;
        if (toRelease > 0)
        {
            m_sema.Signal(toRelease);
        }
    }

    // ------------------------------------------------------------------------

    namespace RWLocks
    {
        enum Attribute : u32
        {
            Readers,
            RWaits,
            Writers,

            COUNT,
            WIDTH = 10u,
        };

        using State = BitPack::Value<COUNT, WIDTH>;

        static constexpr u32 OneReader =
            BitPack::Pack<Readers, WIDTH>(1u);
        static constexpr u32 OneWriter =
            BitPack::Pack<Writers, WIDTH>(1u);
    };

    void RWLock::LockReader()
    {
        using namespace RWLocks;

        State oldState = m_state.Load(MO_Relaxed);
        State newState;
        do
        {
            newState = oldState;
            if (oldState.Get<Writers>() > 0)
            {
                newState.Inc<RWaits>();
            }
            else
            {
                newState.Inc<Readers>();
            }
        } while (!m_state.CmpExWeak(
            oldState,
            newState,
            MO_Acquire,
            MO_Relaxed));

        if (oldState.Get<Writers>() > 0)
        {
            m_read.Wait();
        }
    }

    void RWLock::UnlockReader()
    {
        using namespace RWLocks;

        State oldState = m_state.Sub(OneReader, MO_Release);
        u32 writers = oldState.Get<Writers>();
        u32 readers = oldState.Get<Readers>();
        ASSERT(readers > 0);
        if ((readers == 1) && (writers > 0))
        {
            m_write.Signal();
        }
    }

    void RWLock::LockWriter()
    {
        using namespace RWLocks;

        State oldState = m_state.Add(OneWriter, MO_Acquire);
        u32 writers = oldState.Get<Writers>();
        u32 readers = oldState.Get<Readers>();
        ASSERT(writers != State::MaxValue);
        if ((writers > 0) || (readers > 0))
        {
            m_write.Wait();
        }
    }

    void RWLock::UnlockWriter()
    {
        using namespace RWLocks;

        State oldState = m_state.Load(MO_Relaxed);
        State newState;
        u32 waits = 0u;
        do
        {
            ASSERT(oldState.Get<Readers>() == 0u);
            newState = oldState;
            newState.Dec<Writers>();
            waits = oldState.Get<RWaits>();
            if (waits > 0)
            {
                newState.Set<RWaits>(0);
                newState.Set<Readers>(waits);
            }
        } while (!m_state.CmpExWeak(
            oldState,
            newState,
            MO_Release,
            MO_Relaxed));

        if (waits > 0u)
        {
            m_read.Signal(waits);
        }
        else if (oldState.Get<Writers>() > 1)
        {
            m_write.Signal();
        }
    }

    // ------------------------------------------------------------------------

    void Event::Signal()
    {
        i32 oldState = m_state.Load(MO_Relaxed);
        while (true)
        {
            ASSERT(oldState <= 1);
            i32 newState = Min(oldState + 1, 1);
            if (m_state.CmpExWeak(
                oldState,
                newState,
                MO_Release,
                MO_Relaxed))
            {
                break;
            }
        }
        if (oldState < 0)
        {
            m_sema.Signal();
        }
    }

    void Event::Wait()
    {
        i32 oldState = m_state.Sub(1, MO_Acquire);
        ASSERT(oldState <= 1);
        if (oldState < 1)
        {
            m_sema.Wait();
        }
    }
};
