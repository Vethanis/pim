#pragma once

#include "common/macro.h"
#include "common/int_types.h"

// ----------------------------------------------------------------------------

enum MemOrder : u32
{
    MO_Relaxed = 0u,
    MO_Consume,
    MO_Acquire,
    MO_Release,
    MO_AcqRel,
    MO_SeqCst,
};

void SignalFenceAcquire();
void SignalFenceRelease();

void ThreadFenceAcquire();
void ThreadFenceRelease();

// ----------------------------------------------------------------------------

#define ATOMIC_INTERFACE(T) \
    T Load(const volatile T& atom, MemOrder ord); \
    void Store(volatile T& atom, T x, MemOrder ord); \
    bool CmpExWeak(volatile T& atom, T& expected, T desired, MemOrder success, MemOrder failure); \
    bool CmpExStrong(volatile T& atom, T& expected, T desired, MemOrder success, MemOrder failure); \
    T Exchange(volatile T& atom, T x, MemOrder ord); \
    T Inc(volatile T& atom, MemOrder ord); \
    T Dec(volatile T& atom, MemOrder ord); \
    T FetchAdd(volatile T& atom, T x, MemOrder ord); \
    T FetchSub(volatile T& atom, T x, MemOrder ord); \
    T FetchAnd(volatile T& atom, T x, MemOrder ord); \
    T FetchOr(volatile T& atom, T x, MemOrder ord);

// ----------------------------------------------------------------------------

ATOMIC_INTERFACE(u8);
ATOMIC_INTERFACE(i8);
ATOMIC_INTERFACE(u16);
ATOMIC_INTERFACE(i16);
ATOMIC_INTERFACE(u32);
ATOMIC_INTERFACE(i32);
ATOMIC_INTERFACE(u64);
ATOMIC_INTERFACE(i64);

#undef ATOMIC_INTERFACE

// ----------------------------------------------------------------------------

template<typename T>
struct APtr
{
    using TPtr = T*;

#if PLAT_64
    using OPtr = i64;
#else
    using OPtr = i32;
#endif // PLAT_64

    static TPtr Load(const volatile TPtr& atom, MemOrder ord)
    {
        return (TPtr)::Load((const volatile OPtr&)atom, ord);
    }
    static void Store(volatile TPtr& atom, TPtr x, MemOrder ord)
    {
        ::Store((volatile OPtr&)atom, (OPtr)x, ord);
    }
    static bool CmpExWeak(volatile TPtr& atom, TPtr& expected, TPtr desired, MemOrder success, MemOrder failure)
    {
        return ::CmpExWeak((volatile OPtr&)atom, (OPtr&)expected, (OPtr)desired, success, failure);
    }
    static bool CmpExStrong(volatile TPtr& atom, TPtr& expected, TPtr desired, MemOrder success, MemOrder failure)
    {
        return ::CmpExStrong((volatile OPtr&)atom, (OPtr&)expected, (OPtr)desired, success, failure);
    }
    static TPtr Exchange(volatile TPtr& atom, TPtr x, MemOrder ord)
    {
        return (TPtr)::Exchange((volatile OPtr&)atom, (OPtr)x, ord);
    }
    static TPtr Inc(volatile TPtr& atom, MemOrder ord)
    {
        return (TPtr)::FetchAdd((volatile OPtr&)atom, sizeof(T), ord);
    }
    static TPtr Dec(volatile TPtr& atom, MemOrder ord)
    {
        return (TPtr)::FetchSub((volatile OPtr&)atom, sizeof(T), ord);
    }
    static TPtr FetchAdd(volatile TPtr& atom, OPtr x, MemOrder ord)
    {
        return (TPtr)::FetchAdd((volatile OPtr&)atom, x * sizeof(T), ord);
    }
    static TPtr FetchSub(volatile TPtr& atom, OPtr x, MemOrder ord)
    {
        return (TPtr)::FetchSub((volatile OPtr&)atom, x * sizeof(T), ord);
    }
};
