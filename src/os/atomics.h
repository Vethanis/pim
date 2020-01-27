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
    bool CmpExWeak(volatile T& atom, T& expected, T desired, MemOrder success = MO_AcqRel, MemOrder failure = MO_Relaxed); \
    bool CmpExStrong(volatile T& atom, T& expected, T desired, MemOrder success = MO_AcqRel, MemOrder failure = MO_Relaxed); \
    T Exchange(volatile T& atom, T x, MemOrder ord = MO_AcqRel); \
    T Inc(volatile T& atom, MemOrder ord = MO_AcqRel); \
    T Dec(volatile T& atom, MemOrder ord = MO_AcqRel); \
    T FetchAdd(volatile T& atom, T x, MemOrder ord = MO_AcqRel); \
    T FetchSub(volatile T& atom, T x, MemOrder ord = MO_AcqRel); \
    T FetchAnd(volatile T& atom, T x, MemOrder ord = MO_AcqRel); \
    T FetchOr(volatile T& atom, T x, MemOrder ord = MO_AcqRel);

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

#if PLAT_64
using OPtr = i64;
#else
using OPtr = i32;
#endif // PLAT_64

template<typename T>
static T* LoadPtr(T* const & atom, MemOrder ord = MO_Acquire)
{
    return (T*)::Load((const volatile OPtr&)atom, ord);
}

template<typename T>
static void StorePtr(T*& atom, T* x, MemOrder ord = MO_Release)
{
    ::Store((volatile OPtr&)atom, (OPtr)x, ord);
}

template<typename T>
static bool CmpExWeakPtr(T*& atom, T*& expected, T* desired, MemOrder success = MO_AcqRel, MemOrder failure = MO_Relaxed)
{
    return ::CmpExWeak((volatile OPtr&)atom, (OPtr&)expected, (OPtr)desired, success, failure);
}

template<typename T>
static bool CmpExStrongPtr(T*& atom, T*& expected, T* desired, MemOrder success = MO_AcqRel, MemOrder failure = MO_Relaxed)
{
    return ::CmpExStrong((volatile OPtr&)atom, (OPtr&)expected, (OPtr)desired, success, failure);
}

template<typename T>
static T* ExchangePtr(T*& atom, T* x, MemOrder ord = MO_AcqRel)
{
    return (T*)::Exchange((volatile OPtr&)atom, (OPtr)x, ord);
}

template<typename T>
static T* IncPtr(T*& atom, MemOrder ord)
{
    return (T*)::FetchAdd((volatile OPtr&)atom, sizeof(T), ord);
}

template<typename T>
static T* DecPtr(T*& atom, MemOrder ord)
{
    return (T*)::FetchSub((volatile OPtr&)atom, sizeof(T), ord);
}

template<typename T>
static T* FetchAddPtr(T*& atom, OPtr x, MemOrder ord)
{
    return (T*)::FetchAdd((volatile OPtr&)atom, x * sizeof(T), ord);
}

template<typename T>
static T* FetchSubPtr(T*& atom, OPtr x, MemOrder ord)
{
    return (T*)::FetchSub((volatile OPtr&)atom, x * sizeof(T), ord);
}
