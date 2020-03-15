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

extern "C" void SignalFenceAcquire() { std::atomic_signal_fence(std::memory_order_acquire); }
extern "C" void SignalFenceRelease() { std::atomic_signal_fence(std::memory_order_release); }

extern "C" void ThreadFenceAcquire() { std::atomic_thread_fence(std::memory_order_acquire); }
extern "C" void ThreadFenceRelease() { std::atomic_thread_fence(std::memory_order_release); }

// ----------------------------------------------------------------------------

#define STD_T(T, atom) reinterpret_cast<volatile std::atomic<T>*>(atom)
#define STD_CT(T, atom) reinterpret_cast<const volatile std::atomic<T>*>(atom)

#define STD_E(ord) (std::memory_order)(ord)

#define DEF_ATOMIC(T, tag) \
    extern "C" T load_##tag(const volatile T* atom, memorder_t ord) { return STD_CT(T, atom)->load(STD_E(ord)); } \
    extern "C" void store_##tag(volatile T* atom, T x, memorder_t ord) { STD_T(T, atom)->store(x, STD_E(ord)); } \
    extern "C" int32_t cmpex_##tag(volatile T* atom, T* expected, T desired, memorder_t success) \
    { \
        return STD_T(T, atom)->compare_exchange_strong(*expected, desired, STD_E(success), std::memory_order_relaxed); \
    } \
    extern "C" T exch_##tag(volatile T* atom, T x, memorder_t ord) { return STD_T(T, atom)->exchange(x, STD_E(ord)); } \
    extern "C" T inc_##tag(volatile T* atom, memorder_t ord) { return STD_T(T, atom)->fetch_add(1, STD_E(ord)); } \
    extern "C" T dec_##tag(volatile T* atom, memorder_t ord) { return STD_T(T, atom)->fetch_sub(1, STD_E(ord)); } \
    extern "C" T fetch_add_##tag(volatile T* atom, T x, memorder_t ord) { return STD_T(T, atom)->fetch_add(x, STD_E(ord)); } \
    extern "C" T fetch_sub_##tag(volatile T* atom, T x, memorder_t ord) { return STD_T(T, atom)->fetch_sub(x, STD_E(ord)); } \
    extern "C" T fetch_and_##tag(volatile T* atom, T x, memorder_t ord) { return STD_T(T, atom)->fetch_and(x, STD_E(ord)); } \
    extern "C" T fetch_or_##tag(volatile T* atom, T x, memorder_t ord) { return STD_T(T, atom)->fetch_or(x, STD_E(ord)); } \
    extern "C" T fetch_xor_##tag(volatile T* atom, T x, memorder_t ord) { return STD_T(T, atom)->fetch_xor(x, STD_E(ord)); }

DEF_ATOMIC(uint8_t, u8)
DEF_ATOMIC(int8_t, i8)
DEF_ATOMIC(uint16_t, u16)
DEF_ATOMIC(int16_t, i16)
DEF_ATOMIC(uint32_t, u32)
DEF_ATOMIC(int32_t, i32)
DEF_ATOMIC(uint64_t, u64)
DEF_ATOMIC(int64_t, i64)
DEF_ATOMIC(intptr_t, ptr)

#undef DEF_ATOMIC
