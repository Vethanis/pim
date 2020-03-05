#include "os/atomics.h"

#include "common/macro.h"
#include <atomic>

// https://en.cppreference.com/w/cpp/atomic/atomic_is_lock_free
SASSERT(ATOMIC_BOOL_LOCK_FREE == 2);
SASSERT(ATOMIC_CHAR_LOCK_FREE == 2);
SASSERT(ATOMIC_SHORT_LOCK_FREE == 2);
SASSERT(ATOMIC_INT_LOCK_FREE == 2);
SASSERT(ATOMIC_LONG_LOCK_FREE == 2);
SASSERT(ATOMIC_LLONG_LOCK_FREE == 2);
SASSERT(ATOMIC_POINTER_LOCK_FREE == 2);

SASSERT(MO_Relaxed == std::memory_order_relaxed);
SASSERT(MO_Consume == std::memory_order_consume);
SASSERT(MO_Acquire == std::memory_order_acquire);
SASSERT(MO_Release == std::memory_order_release);
SASSERT(MO_AcqRel == std::memory_order_acq_rel);
SASSERT(MO_SeqCst == std::memory_order_seq_cst);

// ----------------------------------------------------------------------------

void SignalFenceAcquire() { std::atomic_signal_fence(std::memory_order_acquire); }
void SignalFenceRelease() { std::atomic_signal_fence(std::memory_order_release); }

void ThreadFenceAcquire() { std::atomic_thread_fence(std::memory_order_acquire); }
void ThreadFenceRelease() { std::atomic_thread_fence(std::memory_order_release); }

// ----------------------------------------------------------------------------

#define STD_T(T, atom) reinterpret_cast<volatile std::atomic<T>&>(atom)
#define STD_CT(T, atom) reinterpret_cast<const volatile std::atomic<T>&>(atom)

#define STD_E(ord) (std::memory_order)(ord)

#define ATOMIC_IMPL(T) \
    T Load(const volatile T& atom, MemOrder ord) { return STD_CT(T, atom).load(STD_E(ord)); } \
    void Store(volatile T& atom, T x, MemOrder ord) { STD_T(T, atom).store(x, STD_E(ord)); } \
    bool CmpEx(volatile T& atom, T& expected, T desired, MemOrder success, MemOrder failure) \
    { \
        return STD_T(T, atom).compare_exchange_strong(expected, desired, STD_E(success), STD_E(failure)); \
    } \
    T Exchange(volatile T& atom, T x, MemOrder ord) { return STD_T(T, atom).exchange(x, STD_E(ord)); } \
    T Inc(volatile T& atom, MemOrder ord) { return STD_T(T, atom).fetch_add(1, STD_E(ord)); } \
    T Dec(volatile T& atom, MemOrder ord) { return STD_T(T, atom).fetch_sub(1, STD_E(ord)); } \
    T FetchAdd(volatile T& atom, T x, MemOrder ord) { return STD_T(T, atom).fetch_add(x, STD_E(ord)); } \
    T FetchSub(volatile T& atom, T x, MemOrder ord) { return STD_T(T, atom).fetch_sub(x, STD_E(ord)); } \
    T FetchAnd(volatile T& atom, T x, MemOrder ord) { return STD_T(T, atom).fetch_and(x, STD_E(ord)); } \
    T FetchOr(volatile T& atom, T x, MemOrder ord) { return STD_T(T, atom).fetch_or(x, STD_E(ord)); } \
    T FetchXor(volatile T& atom, T x, MemOrder ord) { return STD_T(T, atom).fetch_xor(x, STD_E(ord)); }

ATOMIC_IMPL(u8);
ATOMIC_IMPL(i8);
ATOMIC_IMPL(u16);
ATOMIC_IMPL(i16);
ATOMIC_IMPL(u32);
ATOMIC_IMPL(i32);
ATOMIC_IMPL(u64);
ATOMIC_IMPL(i64);
