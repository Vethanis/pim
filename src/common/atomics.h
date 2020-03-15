#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include <stdint.h>

typedef enum
{
    MO_Relaxed = 0,
    MO_Consume,
    MO_Acquire,
    MO_Release,
    MO_AcqRel,
    MO_SeqCst,
} memorder_t;

void SignalFenceAcquire();
void SignalFenceRelease();

void ThreadFenceAcquire();
void ThreadFenceRelease();

#define DECL_ATOMIC(T, tag) \
    T load_##tag(const volatile T* atom, memorder_t ord); \
    void store_##tag(volatile T* atom, T x, memorder_t ord); \
    int32_t cmpex_##tag(volatile T* atom, T* expected, T desired, memorder_t ord); \
    T exch_##tag(volatile T* atom, T x, memorder_t ord); \
    T inc_##tag(volatile T* atom, memorder_t ord); \
    T dec_##tag(volatile T* atom, memorder_t ord); \
    T fetch_add_##tag(volatile T* atom, T x, memorder_t ord); \
    T fetch_sub_##tag(volatile T* atom, T x, memorder_t ord); \
    T fetch_and_##tag(volatile T* atom, T x, memorder_t ord); \
    T fetch_or_##tag(volatile T* atom, T x, memorder_t ord); \
    T fetch_xor_##tag(volatile T* atom, T x, memorder_t ord);

DECL_ATOMIC(uint8_t, u8)
DECL_ATOMIC(int8_t, i8)
DECL_ATOMIC(uint16_t, u16)
DECL_ATOMIC(int16_t, i16)
DECL_ATOMIC(uint32_t, u32)
DECL_ATOMIC(int32_t, i32)
DECL_ATOMIC(uint64_t, u64)
DECL_ATOMIC(int64_t, i64)
DECL_ATOMIC(intptr_t, ptr)

#define LoadPtr(T, atom, ord)               (T*)load_ptr((const volatile intptr_t*)&(atom), ord)
#define StorePtr(T, atom, x, ord)           store_ptr((volatile intptr_t*)&(atom), (intptr_t)x, ord)
#define CmpExPtr(T, atom, exp, des, ord)    cmpex_ptr((volatile intptr_t*)&(atom), (intptr_t*)&(exp), (intptr_t)des, ord)
#define ExchPtr(T, atom, x, ord)            (T*)exch_ptr((volatile intptr_t*)&(atom), (intptr_t)x, ord)
#define IncPtr(T, atom, ord)                (T*)fetch_add_ptr((volatile intptr_t*)&(atom), sizeof(T), ord)
#define DecPtr(T, atom, ord)                (T*)fetch_sub_ptr((volatile intptr_t*)&(atom), sizeof(T), ord)
#define FetchAddPtr(T, atom, x, ord)        (T*)fetch_add_ptr((volatile intptr_t*)&(atom), sizeof(T) * (intptr_t)(x), ord)
#define FetchSubPtr(T, atom, x, ord)        (T*)fetch_sub_ptr((volatile intptr_t*)&(atom), sizeof(T) * (intptr_t)(x), ord)

PIM_C_END

#undef DECL_ATOMIC

#ifdef ATOMIC_IMPL

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

#endif // ATOMIC_IMPL
