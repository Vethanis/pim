#include "os/atomics.h"

#include "common/macro.h"

#if PLAT_WINDOWS

#include <Windows.h>
#include <intrin.h>

// ----------------------------------------------------------------------------

void SignalFenceAcquire() { _ReadWriteBarrier(); }
void SignalFenceRelease() { _ReadWriteBarrier(); }

void ThreadFenceAcquire() { _ReadWriteBarrier(); }
void ThreadFenceRelease() { _ReadWriteBarrier(); }

// ----------------------------------------------------------------------------

u32 CmpExRelaxed(a32& atom, u32 expected, u32 desired)
{
    return _InterlockedCompareExchange((long*)&atom, desired, expected);
}
u64 CmpExRelaxed(a64& atom, u64 expected, u64 desired)
{
    return _InterlockedCompareExchange64((LONG64*)&atom, desired, expected);
}
ptr_t CmpExRelaxed(aPtr& atom, ptr_t expected, ptr_t desired)
{
    return _InterlockedCompareExchangePointer((PVOID*)&atom, desired, expected);
}

// ----------------------------------------------------------------------------

bool CmpExWeakRelaxed(a32& atom, u32& expected, u32 desired)
{
    u32 e = expected;
    u32 prev = _InterlockedCompareExchange((long*)&atom, desired, e);
    bool matched = (prev == e);
    if (!matched)
    {
        expected = prev;
    }
    return matched;
}
bool CmpExWeakRelaxed(a64& atom, u64& expected, u64 desired)
{
    u64 e = expected;
    u64 prev = _InterlockedCompareExchange64((LONG64*)&atom, desired, e);
    bool matched = (prev == e);
    if (!matched)
    {
        expected = prev;
    }
    return matched;
}
bool CmpExWeakRelaxed(aPtr& atom, ptr_t& expected, ptr_t desired)
{
    ptr_t e = expected;
    ptr_t prev = _InterlockedCompareExchangePointer((PVOID*)&atom, desired, e);
    bool matched = (prev == e);
    if (!matched)
    {
        expected = prev;
    }
    return matched;
}

// ----------------------------------------------------------------------------

u32 ExchangeRelaxed(a32& atom, u32 desired)
{
    return _InterlockedExchange((long*)&atom, desired);
}
u64 ExchangeRelaxed(a64& atom, u64 desired)
{
    return _InterlockedExchange64((LONG64*)&atom, desired);
}
ptr_t ExchangeRelaxed(aPtr& atom, ptr_t desired)
{
    return _InterlockedExchangePointer((PVOID*)&atom, desired);
}

// ----------------------------------------------------------------------------

u32 FetchAddRelaxed(a32& atom, u32 x)
{
    return _InterlockedExchangeAdd((long*)&atom, x);
}
u64 FetchAddRelaxed(a64& atom, u64 x)
{
    return _InterlockedExchangeAdd64((LONG64*)&atom, x);
}
ptr_t FetchAddRelaxed(aPtr& atom, isize x)
{
#ifdef PLAT_64
    return (PVOID)_InterlockedExchangeAdd64((LONG64*)&atom, x);
#else
    return (PVOID)_InterlockedExchangeAdd((long*)&atom, x);
#endif
}

// ----------------------------------------------------------------------------

u32 IncRelaxed(a32& atom)
{
    return _InterlockedIncrement((long*)&atom);
}
u64 IncRelaxed(a64& atom)
{
    return _InterlockedIncrement64((LONG64*)&atom);
}

// ----------------------------------------------------------------------------

u32 DecRelaxed(a32& atom)
{
    return _InterlockedDecrement((long*)&atom);
}
u64 DecRelaxed(a64& atom)
{
    return _InterlockedDecrement64((LONG64*)&atom);
}

// ----------------------------------------------------------------------------

u32 FetchAndRelaxed(a32& atom, u32 x)
{
    return _InterlockedAnd((long*)&atom, x);
}
u64 FetchAndRelaxed(a64& atom, u64 x)
{
    return _InterlockedAnd64((LONG64*)&atom, x);
}

// ----------------------------------------------------------------------------

u32 FetchOrRelaxed(a32& atom, u32 x)
{
    return _InterlockedOr((long*)&atom, x);
}
u64 FetchOrRelaxed(a64& atom, u64 x)
{
    return _InterlockedOr64((LONG64*)&atom, x);
}

// ----------------------------------------------------------------------------

#endif // PLAT_WINDOWS
