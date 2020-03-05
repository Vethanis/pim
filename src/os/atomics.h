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
    T Load(const volatile T& atom, MemOrder ord = MO_Acquire); \
    void Store(volatile T& atom, T x, MemOrder ord = MO_Release); \
    bool CmpEx(volatile T& atom, T& expected, T desired, MemOrder success = MO_Acquire, MemOrder failure = MO_Relaxed); \
    T Exchange(volatile T& atom, T x, MemOrder ord = MO_Relaxed); \
    T Inc(volatile T& atom, MemOrder ord = MO_Relaxed); \
    T Dec(volatile T& atom, MemOrder ord = MO_Relaxed); \
    T FetchAdd(volatile T& atom, T x, MemOrder ord = MO_Relaxed); \
    T FetchSub(volatile T& atom, T x, MemOrder ord = MO_Relaxed); \
    T FetchAnd(volatile T& atom, T x, MemOrder ord = MO_Relaxed); \
    T FetchOr(volatile T& atom, T x, MemOrder ord = MO_Relaxed); \
    T FetchXor(volatile T& atom, T x, MemOrder ord = MO_Relaxed);

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
static T* LoadPtr(T* const & atom, MemOrder ord = MO_Acquire)
{
    return (T*)::Load((const volatile isize&)atom, ord);
}

template<typename T>
static void StorePtr(T*& atom, T* x, MemOrder ord = MO_Release)
{
    ::Store((volatile isize&)atom, (isize)x, ord);
}

template<typename T>
static bool CmpExPtr(T*& atom, T*& expected, T* desired, MemOrder success = MO_Acquire, MemOrder failure = MO_Relaxed)
{
    return ::CmpEx((volatile isize&)atom, (isize&)expected, (isize)desired, success, failure);
}

template<typename T>
static T* ExchangePtr(T*& atom, T* x, MemOrder ord = MO_AcqRel)
{
    return (T*)::Exchange((volatile isize&)atom, (isize)x, ord);
}

template<typename T>
static T* IncPtr(T*& atom, MemOrder ord = MO_Relaxed)
{
    return (T*)::FetchAdd((volatile isize&)atom, sizeof(T), ord);
}

template<typename T>
static T* DecPtr(T*& atom, MemOrder ord = MO_Relaxed)
{
    return (T*)::FetchSub((volatile isize&)atom, sizeof(T), ord);
}

template<typename T>
static T* FetchAddPtr(T*& atom, isize x, MemOrder ord = MO_Relaxed)
{
    return (T*)::FetchAdd((volatile isize&)atom, x * sizeof(T), ord);
}

template<typename T>
static T* FetchSubPtr(T*& atom, isize x, MemOrder ord = MO_Relaxed)
{
    return (T*)::FetchSub((volatile isize&)atom, x * sizeof(T), ord);
}
