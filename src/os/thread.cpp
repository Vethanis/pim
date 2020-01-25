#include "os/thread.h"

#include "common/macro.h"
#include "common/minmax.h"
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
            if (spins > 10)
            {
                OS::Rest(0);
                spins = 0;
            }
            else
            {
                Spin(spins * 100);
            }
        }
    }

    void SpinLock::Unlock()
    {
        i32 prev = Exchange(m_count, 0, MO_Release);
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
            Spin(spins * 100);
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

    struct RWState
    {
        u32 readers : 10;
        u32 waiters : 10;
        u32 writers : 10;

        static constexpr u32 kMask = (1u << 10) - 1u;

        RWState(u32 x = 0u)
        {
            *this = *reinterpret_cast<RWState*>(&x);
        }

        operator u32&()
        {
            return *reinterpret_cast<u32*>(this);
        }

        void IncWait()
        {
            u32 x = waiters;
            ASSERT(x < kMask);
            waiters = (x + 1u) & kMask;
        }
        void DecWait()
        {
            u32 x = waiters;
            ASSERT(x > 0u);
            waiters = (x - 1u) & kMask;
        }

        void IncWriter()
        {
            u32 x = writers;
            ASSERT(x < kMask);
            writers = (x + 1u) & kMask;
        }
        void DecWriter()
        {
            u32 x = writers;
            ASSERT(x > 0u);
            writers = (x - 1u) & kMask;
        }

        void IncReader()
        {
            u32 x = readers;
            ASSERT(x < kMask);
            readers = (x + 1u) & kMask;
        }
        void DecReader()
        {
            u32 x = readers;
            ASSERT(x > 0u);
            readers = (x - 1u) & kMask;
        }
    };
    SASSERT(sizeof(RWState) == sizeof(u32));
    SASSERT(alignof(RWState) == alignof(u32));

    void RWLock::LockReader()
    {
        RWState oldState = Load(m_state, MO_Relaxed);
        RWState newState;
        do
        {
            newState = oldState;
            if (newState.writers)
            {
                newState.IncWait();
            }
            else
            {
                newState.IncReader();
            }
        } while (!CmpExStrong(m_state, oldState, newState, MO_Acquire, MO_Relaxed));

        if (oldState.writers != 0u)
        {
            m_read.Wait();
        }
    }

    void RWLock::UnlockReader()
    {
        RWState oneReader = 0u;
        oneReader.IncReader();
        RWState oldState = FetchSub(m_state, oneReader, MO_Release);
        ASSERT(oldState.readers);
        if ((oldState.readers == 1u) && (oldState.writers != 0u))
        {
            m_write.Signal();
        }
    }

    void RWLock::LockWriter()
    {
        RWState oneWriter = 0u;
        oneWriter.IncWriter();
        RWState oldState = FetchAdd(m_state, oneWriter, MO_Acquire);
        ASSERT(oldState.writers < RWState::kMask);
        if (oldState.writers || oldState.readers)
        {
            m_write.Wait();
        }
    }

    void RWLock::UnlockWriter()
    {
        RWState oldState = Load(m_state, MO_Relaxed);
        RWState newState;
        u32 waits = 0u;
        do
        {
            newState = oldState;
            newState.DecWriter();
            waits = newState.waiters;
            if (waits != 0u)
            {
                newState.waiters = 0u;
                newState.readers = waits;
            }
        } while (!CmpExWeak(m_state, oldState, newState, MO_Release, MO_Relaxed));

        if (waits)
        {
            m_read.Signal(waits);
        }
        else if (oldState.writers > 1u)
        {
            m_write.Signal();
        }
    }

    // ------------------------------------------------------------------------

    void Event::WakeOne()
    {
        i32 waits = Load(m_waits, MO_Relaxed);
        while (!CmpExStrong(m_waits, waits, Max(waits - 1, 0), MO_Release, MO_Relaxed))
        {
            OS::YieldCore();
        }
        if (waits > 0)
        {
            m_sema.Signal(1);
        }
    }

    void Event::WakeAll()
    {
        i32 waits = Load(m_waits, MO_Relaxed);
        while (!CmpExStrong(m_waits, waits, 0, MO_Release, MO_Relaxed))
        {
            OS::YieldCore();
        }
        if (waits > 0)
        {
            m_sema.Signal(waits);
        }
    }

    void Event::Wait()
    {
        Inc(m_waits, MO_Acquire);
        m_sema.Wait();
    }
};
