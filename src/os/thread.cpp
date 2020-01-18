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
            Spin(spins * 100);
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
        } while (!CmpExStrong(m_state, oldState, newState, MO_AcqRel, MO_Relaxed));

        if (oldState.writers != 0u)
        {
            m_read.Wait();
        }
    }

    void RWLock::UnlockReader()
    {
        RWState oldState = Load(m_state, MO_Relaxed);
        RWState newState;
        do
        {
            newState = oldState;
            newState.DecReader();
        } while (!CmpExStrong(m_state, oldState, newState, MO_AcqRel, MO_Relaxed));

        if ((oldState.readers == 1u) && (oldState.writers != 0u))
        {
            m_write.Signal();
        }
    }

    void RWLock::LockWriter()
    {
        RWState oldState = Load(m_state, MO_Relaxed);
        RWState newState;
        do
        {
            newState = oldState;
            newState.IncWriter();
        } while (!CmpExStrong(m_state, oldState, newState, MO_AcqRel, MO_Relaxed));

        if ((oldState.writers != 0u) || (oldState.readers != 0u))
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
        } while (!CmpExWeak(m_state, oldState, newState, MO_AcqRel, MO_Relaxed));

        if (waits != 0u)
        {
            m_read.Signal(waits);
        }
        else if (oldState.writers > 1u)
        {
            m_write.Signal();
        }
    }

    // ------------------------------------------------------------------------

    void Event::Signal()
    {
        i32 oldState = Load(m_state, MO_Relaxed);
        while (!CmpExStrong(m_state, oldState, Min(oldState + 1, 1), MO_Acquire, MO_Relaxed))
        {
            OS::YieldCore();
        }
        if (oldState < 0)
        {
            m_sema.Signal();
        }
    }

    void Event::Wait()
    {
        i32 oldState = Dec(m_state, MO_Acquire);
        if (oldState < 1)
        {
            m_sema.Wait();
        }
    }

    // ------------------------------------------------------------------------

    void MultiEvent::Signal()
    {
        i32 oldCount = Load(m_waitCount, MO_Relaxed);
        if (oldCount > 0)
        {
            if (CmpExStrong(m_waitCount, oldCount, 0, MO_Acquire, MO_Relaxed))
            {
                m_sema.Signal(oldCount);
            }
        }
    }

    void MultiEvent::Wait()
    {
        Inc(m_waitCount, MO_Acquire);
        m_sema.Wait();
    }
};
