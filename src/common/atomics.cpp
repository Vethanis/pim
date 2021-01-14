#include "common/atomics.h"

#include <atomic>

// https://en.cppreference.com/w/cpp/atomic/atomic_is_lock_free
SASSERT(ATOMIC_BOOL_LOCK_FREE == 2);
SASSERT(ATOMIC_CHAR_LOCK_FREE == 2);
SASSERT(ATOMIC_SHORT_LOCK_FREE == 2);
SASSERT(ATOMIC_INT_LOCK_FREE == 2);
SASSERT(ATOMIC_LONG_LOCK_FREE == 2);
SASSERT(ATOMIC_LLONG_LOCK_FREE == 2);
SASSERT(ATOMIC_POINTER_LOCK_FREE == 2);

SASSERT((int)MO_Relaxed == (int)std::memory_order_relaxed);
SASSERT((int)MO_Consume == (int)std::memory_order_consume);
SASSERT((int)MO_Acquire == (int)std::memory_order_acquire);
SASSERT((int)MO_Release == (int)std::memory_order_release);
SASSERT((int)MO_AcqRel == (int)std::memory_order_acq_rel);
SASSERT((int)MO_SeqCst == (int)std::memory_order_seq_cst);

// ----------------------------------------------------------------------------

extern "C" void SignalFenceAcquire() { std::atomic_signal_fence(std::memory_order_acquire); }
extern "C" void SignalFenceRelease() { std::atomic_signal_fence(std::memory_order_release); }

extern "C" void ThreadFenceAcquire() { std::atomic_thread_fence(std::memory_order_acquire); }
extern "C" void ThreadFenceRelease() { std::atomic_thread_fence(std::memory_order_release); }

// ----------------------------------------------------------------------------

#define STD_T(T, atom) reinterpret_cast<volatile std::atomic<T>*>(atom)
#define STD_CT(T, atom) reinterpret_cast<const volatile std::atomic<T>*>(atom)

#define STD_E(ord) (std::memory_order)(ord)

#define DEF_ATOMIC(T) \
    extern "C" T load_##T(const volatile T* atom, memorder_t ord) { return STD_CT(T, atom)->load(STD_E(ord)); } \
    extern "C" void store_##T(volatile T* atom, T x, memorder_t ord) { STD_T(T, atom)->store(x, STD_E(ord)); } \
    extern "C" i32 cmpex_##T(volatile T* atom, T* expected, T desired, memorder_t success) \
    { \
        return STD_T(T, atom)->compare_exchange_strong(*expected, desired, STD_E(success), std::memory_order_relaxed); \
    } \
    extern "C" T exch_##T(volatile T* atom, T x, memorder_t ord) { return STD_T(T, atom)->exchange(x, STD_E(ord)); } \
    extern "C" T inc_##T(volatile T* atom, memorder_t ord) { return STD_T(T, atom)->fetch_add(1, STD_E(ord)); } \
    extern "C" T dec_##T(volatile T* atom, memorder_t ord) { return STD_T(T, atom)->fetch_sub(1, STD_E(ord)); } \
    extern "C" T fetch_add_##T(volatile T* atom, T x, memorder_t ord) { return STD_T(T, atom)->fetch_add(x, STD_E(ord)); } \
    extern "C" T fetch_sub_##T(volatile T* atom, T x, memorder_t ord) { return STD_T(T, atom)->fetch_sub(x, STD_E(ord)); } \
    extern "C" T fetch_and_##T(volatile T* atom, T x, memorder_t ord) { return STD_T(T, atom)->fetch_and(x, STD_E(ord)); } \
    extern "C" T fetch_or_##T(volatile T* atom, T x, memorder_t ord) { return STD_T(T, atom)->fetch_or(x, STD_E(ord)); } \
    extern "C" T fetch_xor_##T(volatile T* atom, T x, memorder_t ord) { return STD_T(T, atom)->fetch_xor(x, STD_E(ord)); }

DEF_ATOMIC(u8)
DEF_ATOMIC(i8)
DEF_ATOMIC(u16)
DEF_ATOMIC(i16)
DEF_ATOMIC(u32)
DEF_ATOMIC(i32)
DEF_ATOMIC(u64)
DEF_ATOMIC(i64)
DEF_ATOMIC(isize)
