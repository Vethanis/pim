#include "os/thread.h"

#include "common/macro.h"
#include "common/minmax.h"
#include "os/atomics.h"

namespace OS
{
    struct RWState
    {
        u32 readers : 10;
        u32 waiters : 10;
        u32 writers : 10;

        static constexpr u32 kMask = (1u << 10) - 1u;

        static u32 Inc(u32 x) { ASSERT(x < kMask); return (x + 1u) & kMask; }
        static u32 Dec(u32 x) { ASSERT(x); return (x - 1u) & kMask; }

        RWState(u32 x = 0u) { *this = *reinterpret_cast<RWState*>(&x); }
        operator u32&() { return *reinterpret_cast<u32*>(this); }

        void IncWait() { waiters = Inc(waiters); }
        void DecWait() { waiters = Dec(waiters); }

        void IncWriter() { writers = Inc(writers); }
        void DecWriter() { writers = Dec(writers); }

        void IncReader() { readers = Inc(readers); }
        void DecReader() { readers = Dec(readers); }
    };
    SASSERT(sizeof(RWState) == sizeof(u32));
    SASSERT(alignof(RWState) == alignof(u32));

    void RWLock::LockReader() const
    {
        u64 spins = 0;
    trywrite:
        RWState oldState = Load(m_state, MO_Relaxed);
        RWState newState = oldState;
        if (newState.writers)
        {
            newState.IncWait();
        }
        else
        {
            newState.IncReader();
        }
        if (!CmpExStrong(m_state, oldState, newState, MO_Acquire))
        {
            OS::Spin(++spins);
            goto trywrite;
        }
        if (oldState.writers)
        {
            m_read.Wait();
        }
    }

    void RWLock::UnlockReader() const
    {
        RWState oneReader = 0u;
        oneReader.readers = 1u;
        RWState oldState = FetchSub(m_state, oneReader, MO_Release);
        ASSERT(oldState.readers);
        if ((oldState.readers == 1u) && oldState.writers)
        {
            m_write.Signal();
        }
    }

    void RWLock::LockWriter()
    {
        RWState oneWriter = 0u;
        oneWriter.writers = 1u;
        RWState oldState = FetchAdd(m_state, oneWriter, MO_Acquire);
        ASSERT(oldState.writers < RWState::kMask);
        if (oldState.writers | oldState.readers)
        {
            m_write.Wait();
        }
    }

    void RWLock::UnlockWriter()
    {
        u64 spins = 0;
    trywrite:
        RWState oldState = Load(m_state, MO_Relaxed);
        RWState newState = oldState;
        const u32 waits = newState.waiters;
        if (waits)
        {
            newState.waiters = 0u;
            newState.readers = waits;
        }
        newState.DecWriter();
        if (!CmpExStrong(m_state, oldState, newState, MO_Release))
        {
            OS::Spin(++spins);
            goto trywrite;
        }

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
        u64 spins = 0;
        i32 waits = Load(m_waits);
        while (!CmpExStrong(m_waits, waits, Max(waits - 1, 0), MO_Release))
        {
            OS::Spin(++spins);
        }
        if (waits > 0)
        {
            m_sema.Signal(1);
        }
    }

    void Event::Wake(i32 count)
    {
        u64 spins = 0;
        i32 waits = Load(m_waits);
        while (!CmpExStrong(m_waits, waits, Max(waits - count, 0), MO_Release))
        {
            OS::Spin(++spins);
        }
        if (waits > 0)
        {
            m_sema.Signal(Min(waits, count));
        }
    }

    void Event::WakeAll()
    {
        u64 spins = 0;
        i32 waits = Load(m_waits);
        while (!CmpExStrong(m_waits, waits, 0, MO_Release))
        {
            OS::Spin(++spins);
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

    // ------------------------------------------------------------------------

    bool RWFlag::TryLockReader() const
    {
        i32 state = Load(m_state, MO_Relaxed);
        return (state >= 0) && CmpExStrong(m_state, state, state + 1, MO_Acquire);
    }

    void RWFlag::LockReader() const
    {
        u64 spins = 0;
        while (!TryLockReader())
        {
            OS::Spin(++spins);
        }
    }

    void RWFlag::UnlockReader() const
    {
        i32 prev = Dec(m_state, MO_Release);
        ASSERT(prev > 0);
    }

    bool RWFlag::TryLockWriter()
    {
        i32 state = Load(m_state, MO_Relaxed);
        return (state == 0) && CmpExStrong(m_state, state, -1, MO_Acquire);
    }

    void RWFlag::LockWriter()
    {
        u64 spins = 0;
        while (!TryLockWriter())
        {
            OS::Spin(++spins);
        }
    }

    void RWFlag::UnlockWriter()
    {
        i32 prev = Exchange(m_state, 0, MO_Release);
        ASSERT(prev == -1);
    }
};
