#include "os/thread.h"

#include "common/macro.h"
#include "common/minmax.h"
#include "containers/bitpack.h"
#include "os/atomics.h"

namespace OS
{
    bool SpinLock::TryLock()
    {
        i32 cmp = 0;
        return CmpExStrong(m_count, cmp, 1, MO_Acquire, MO_Relaxed);
    }

    void SpinLock::Lock()
    {
        u64 spins = 0;
        while (!TryLock())
        {
            ++spins;
            SpinWait(spins * 100);
        }
    }

    void SpinLock::Unlock()
    {
        u32 prev = Exchange(m_count, 0, MO_Release);
        ASSERT(prev == 1);
    }

    // ------------------------------------------------------------------------

    bool LightSema::TryWait()
    {
        i32 oldCount = Load(m_count, MO_Relaxed);
        return (oldCount > 0) && CmpExStrong(m_count, oldCount, oldCount - 1, MO_Acquire, MO_Relaxed);
    }

    void LightSema::Wait()
    {
        u64 spins = 0;
        while (spins < 10)
        {
            if (TryWait())
            {
                return;
            }
            ++spins;
            SpinWait(spins * 100);
        }
        i32 oldCount = Dec(m_count, MO_Acquire);
        if (oldCount <= 0)
        {
            m_sema.Wait();
        }
    }

    void LightSema::Signal(i32 count)
    {
        i32 oldCount = FetchAdd(m_count, count, MO_Release);
        i32 toRelease = Min(-oldCount, count);
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

        State oldState = Load(m_state, MO_Relaxed);
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
        } while (!CmpExStrong(m_state, (u32&)oldState, newState, MO_Acquire, MO_Relaxed));

        if (oldState.Get<Writers>() > 0)
        {
            m_read.Wait();
        }
    }

    void RWLock::UnlockReader()
    {
        using namespace RWLocks;

        State oldState = FetchAdd(m_state, 0u - OneReader, MO_Release);
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

        State oldState = FetchAdd(m_state, OneWriter, MO_Acquire);
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

        State oldState = Load(m_state, MO_Relaxed);
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
        } while (!CmpExWeak(m_state, (u32&)oldState, newState, MO_Release, MO_Relaxed));

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
        i32 oldState = Load(m_state, MO_Relaxed);
        while (true)
        {
            ASSERT(oldState <= 1u);
            i32 newState = Min(oldState + 1, 1);
            if (CmpExStrong(m_state, oldState, newState, MO_Release, MO_Relaxed))
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
        i32 oldState = Dec(m_state, MO_Acquire);
        ASSERT(oldState <= 1);
        if (oldState < 1)
        {
            m_sema.Wait();
        }
    }
};
