#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef enum
{
    MO_Relaxed = 0,
    MO_Consume,
    MO_Acquire,
    MO_Release,
    MO_AcqRel,
    MO_SeqCst,
} memorder_t;

void ThreadFenceAcquire();
void ThreadFenceRelease();

#define DECL_ATOMIC(T) \
    T load_##T(const volatile T* atom, memorder_t ord); \
    void store_##T(volatile T* atom, T x, memorder_t ord); \
    i32 cmpex_##T(volatile T* atom, T* expected, T desired, memorder_t ord); \
    T exch_##T(volatile T* atom, T x, memorder_t ord); \
    T inc_##T(volatile T* atom, memorder_t ord); \
    T dec_##T(volatile T* atom, memorder_t ord); \
    T fetch_add_##T(volatile T* atom, T x, memorder_t ord); \
    T fetch_sub_##T(volatile T* atom, T x, memorder_t ord); \
    T fetch_and_##T(volatile T* atom, T x, memorder_t ord); \
    T fetch_or_##T(volatile T* atom, T x, memorder_t ord); \
    T fetch_xor_##T(volatile T* atom, T x, memorder_t ord);

DECL_ATOMIC(u8)
DECL_ATOMIC(i8)
DECL_ATOMIC(u16)
DECL_ATOMIC(i16)
DECL_ATOMIC(u32)
DECL_ATOMIC(i32)
DECL_ATOMIC(u64)
DECL_ATOMIC(i64)
DECL_ATOMIC(isize)

#define LoadPtr(T, atom, ord)               (T*)load_isize((const volatile isize*)&(atom), ord)
#define StorePtr(T, atom, x, ord)           store_isize((volatile isize*)&(atom), (isize)x, ord)
#define CmpExPtr(T, atom, ex, des, ord)     cmpex_isize((volatile isize*)&(atom), (isize*)&(ex), (isize)des, ord)
#define ExchPtr(T, atom, x, ord)            (T*)exch_isize((volatile isize*)&(atom), (isize)x, ord)
#define IncPtr(T, atom, ord)                (T*)fetch_add_isize((volatile isize*)&(atom), sizeof(T), ord)
#define DecPtr(T, atom, ord)                (T*)fetch_sub_isize((volatile isize*)&(atom), sizeof(T), ord)
#define FetchAddPtr(T, atom, x, ord)        (T*)fetch_add_isize((volatile isize*)&(atom), sizeof(T) * (isize)(x), ord)
#define FetchSubPtr(T, atom, x, ord)        (T*)fetch_sub_isize((volatile isize*)&(atom), sizeof(T) * (isize)(x), ord)

PIM_C_END
