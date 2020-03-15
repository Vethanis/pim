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
