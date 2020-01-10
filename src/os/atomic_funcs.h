#pragma once

void SignalFenceAcquire();
void SignalFenceRelease();

void ThreadFenceAcquire();
void ThreadFenceRelease();

inline u32 LoadRelaxed(const a32& x)
{
    return ((volatile a32&)x).Value;
}

inline u64 LoadRelaxed(const a64& x)
{
    return ((volatile a64&)x).Value;
}

inline ptr_t LoadRelaxed(const aPtr& x)
{
    return ((volatile aPtr&)x).Value;
}

inline void StoreRelaxed(a32& dst, u32 value)
{
    ((volatile a32&)dst).Value = value;
}

inline void StoreRelaxed(a64& dst, u64 value)
{
    ((volatile a64&)dst).Value = value;
}

inline void StoreRelaxed(aPtr& dst, ptr_t value)
{
    ((volatile aPtr&)dst).Value = value;
}

u32 CmpExRelaxed(a32& atom, u32 expected, u32 desired);
u64 CmpExRelaxed(a64& atom, u64 expected, u64 desired);
ptr_t CmpExRelaxed(aPtr& atom, ptr_t expected, ptr_t desired);

bool CmpExWeakRelaxed(a32& atom, u32& expected, u32 desired);
bool CmpExWeakRelaxed(a64& atom, u64& expected, u64 desired);
bool CmpExWeakRelaxed(aPtr& atom, ptr_t& expected, ptr_t desired);

u32 ExchangeRelaxed(a32& atom, u32 desired);
u64 ExchangeRelaxed(a64& atom, u64 desired);
ptr_t ExchangeRelaxed(aPtr& atom, ptr_t desired);

u32 FetchAddRelaxed(a32& atom, u32 x);
u64 FetchAddRelaxed(a64& atom, u64 x);
ptr_t FetchAddRelaxed(aPtr& atom, isize x);

u32 IncRelaxed(a32& atom);
u64 IncRelaxed(a64& atom);

u32 DecRelaxed(a32& atom);
u64 DecRelaxed(a64& atom);

u32 FetchAndRelaxed(a32& atom, u32 x);
u64 FetchAndRelaxed(a64& atom, u64 x);

u32 FetchOrRelaxed(a32& atom, u32 x);
u64 FetchOrRelaxed(a64& atom, u64 x);

// ----------------------------------------------------------------------------

inline u32 Load(const a32& atom, MemOrder ord)
{
    u32 result = LoadRelaxed(atom);
    if (ord & MO_Acquire)
    {
        ThreadFenceAcquire();
    }
    return result;
}

inline void Store(a32& atom, u32 x, MemOrder ord)
{
    if (ord & MO_Release)
    {
        ThreadFenceRelease();
    }
    StoreRelaxed(atom, x);
}

inline u32 CmpEx(a32& atom, u32 expected, u32 desired, MemOrder ord)
{
    if (ord & MO_Release)
    {
        ThreadFenceRelease();
    }
    u32 result = CmpExRelaxed(atom, expected, desired);
    if (ord & MO_Acquire)
    {
        ThreadFenceAcquire();
    }
    return result;
}

inline bool CmpExWeak(
    a32& atom, u32& expected, u32 desired,
    MemOrder success, MemOrder failure)
{
    if ((success | failure) & MO_Release)
    {
        ThreadFenceRelease();
    }
    bool result = CmpExWeakRelaxed(atom, expected, desired);
    if (result)
    {
        if (success & MO_Acquire)
        {
            ThreadFenceAcquire();
        }
    }
    else
    {
        if (failure & MO_Acquire)
        {
            ThreadFenceAcquire();
        }
    }
    return result;
}

inline u32 Exchange(a32& atom, u32 x, MemOrder ord)
{
    if (ord & MO_Release)
    {
        ThreadFenceRelease();
    }
    u32 result = ExchangeRelaxed(atom, x);
    if (ord & MO_Acquire)
    {
        ThreadFenceAcquire();
    }
    return result;
}

inline u32 Inc(a32& atom, MemOrder ord)
{
    if (ord & MO_Release)
    {
        ThreadFenceRelease();
    }
    u32 result = IncRelaxed(atom);
    if (ord & MO_Acquire)
    {
        ThreadFenceAcquire();
    }
    return result;
}

inline u32 Dec(a32& atom, MemOrder ord)
{
    if (ord & MO_Release)
    {
        ThreadFenceRelease();
    }
    u32 result = DecRelaxed(atom);
    if (ord & MO_Acquire)
    {
        ThreadFenceAcquire();
    }
    return result;
}

inline u32 FetchAdd(a32& atom, u32 x, MemOrder ord)
{
    if (ord & MO_Release)
    {
        ThreadFenceRelease();
    }
    u32 result = FetchAddRelaxed(atom, x);
    if (ord & MO_Acquire)
    {
        ThreadFenceAcquire();
    }
    return result;
}

inline u32 FetchAnd(a32& atom, u32 x, MemOrder ord)
{
    if (ord & MO_Release)
    {
        ThreadFenceRelease();
    }
    u32 result = FetchAndRelaxed(atom, x);
    if (ord & MO_Acquire)
    {
        ThreadFenceAcquire();
    }
    return result;
}

inline u32 FetchOr(a32& atom, u32 x, MemOrder ord)
{
    if (ord & MO_Release)
    {
        ThreadFenceRelease();
    }
    u32 result = FetchOrRelaxed(atom, x);
    if (ord & MO_Acquire)
    {
        ThreadFenceAcquire();
    }
    return result;
}

// ----------------------------------------------------------------------------

inline u64 Load(const a64& atom, MemOrder ord)
{
    u64 result = LoadRelaxed(atom);
    if (ord & MO_Acquire)
    {
        ThreadFenceAcquire();
    }
    return result;
}

inline void Store(a64& atom, u64 x, MemOrder ord)
{
    if (ord & MO_Release)
    {
        ThreadFenceRelease();
    }
    StoreRelaxed(atom, x);
}

inline u64 CmpEx(a64& atom, u64 expected, u64 desired, MemOrder ord)
{
    if (ord & MO_Release)
    {
        ThreadFenceRelease();
    }
    u64 result = CmpExRelaxed(atom, expected, desired);
    if (ord & MO_Acquire)
    {
        ThreadFenceAcquire();
    }
    return result;
}

inline bool CmpExWeak(
    a64& atom, u64& expected, u64 desired,
    MemOrder success, MemOrder failure)
{
    if ((success | failure) & MO_Release)
    {
        ThreadFenceRelease();
    }
    bool result = CmpExWeakRelaxed(atom, expected, desired);
    if (result)
    {
        if (success & MO_Acquire)
        {
            ThreadFenceAcquire();
        }
    }
    else
    {
        if (failure & MO_Acquire)
        {
            ThreadFenceAcquire();
        }
    }
    return result;
}

inline u64 Exchange(a64& atom, u64 x, MemOrder ord)
{
    if (ord & MO_Release)
    {
        ThreadFenceRelease();
    }
    u64 result = ExchangeRelaxed(atom, x);
    if (ord & MO_Acquire)
    {
        ThreadFenceAcquire();
    }
    return result;
}

inline u64 Inc(a64& atom, MemOrder ord)
{
    if (ord & MO_Release)
    {
        ThreadFenceRelease();
    }
    u64 result = IncRelaxed(atom);
    if (ord & MO_Acquire)
    {
        ThreadFenceAcquire();
    }
    return result;
}

inline u64 Dec(a64& atom, MemOrder ord)
{
    if (ord & MO_Release)
    {
        ThreadFenceRelease();
    }
    u64 result = DecRelaxed(atom);
    if (ord & MO_Acquire)
    {
        ThreadFenceAcquire();
    }
    return result;
}

inline u64 FetchAdd(a64& atom, u64 x, MemOrder ord)
{
    if (ord & MO_Release)
    {
        ThreadFenceRelease();
    }
    u64 result = FetchAddRelaxed(atom, x);
    if (ord & MO_Acquire)
    {
        ThreadFenceAcquire();
    }
    return result;
}

inline u64 FetchAnd(a64& atom, u64 x, MemOrder ord)
{
    if (ord & MO_Release)
    {
        ThreadFenceRelease();
    }
    u64 result = FetchAndRelaxed(atom, x);
    if (ord & MO_Acquire)
    {
        ThreadFenceAcquire();
    }
    return result;
}

inline u64 FetchOr(a64& atom, u64 x, MemOrder ord)
{
    if (ord & MO_Release)
    {
        ThreadFenceRelease();
    }
    u64 result = FetchOrRelaxed(atom, x);
    if (ord & MO_Acquire)
    {
        ThreadFenceAcquire();
    }
    return result;
}

// ----------------------------------------------------------------------------

inline ptr_t Load(const aPtr& atom, MemOrder ord)
{
    ptr_t result = LoadRelaxed(atom);
    if (ord & MO_Acquire)
    {
        ThreadFenceAcquire();
    }
    return result;
}

inline void Store(aPtr& atom, ptr_t x, MemOrder ord)
{
    if (ord & MO_Release)
    {
        ThreadFenceRelease();
    }
    StoreRelaxed(atom, x);
}

inline ptr_t CmpEx(aPtr& atom, ptr_t expected, ptr_t desired, MemOrder ord)
{
    if (ord & MO_Release)
    {
        ThreadFenceRelease();
    }
    ptr_t result = CmpExRelaxed(atom, expected, desired);
    if (ord & MO_Acquire)
    {
        ThreadFenceAcquire();
    }
    return result;
}

inline bool CmpExWeak(
    aPtr& atom, ptr_t& expected, ptr_t desired,
    MemOrder success, MemOrder failure)
{
    if ((success | failure) & MO_Release)
    {
        ThreadFenceRelease();
    }
    bool result = CmpExWeakRelaxed(atom, expected, desired);
    if (result)
    {
        if (success & MO_Acquire)
        {
            ThreadFenceAcquire();
        }
    }
    else
    {
        if (failure & MO_Acquire)
        {
            ThreadFenceAcquire();
        }
    }
    return result;
}

inline ptr_t Exchange(aPtr& atom, ptr_t x, MemOrder ord)
{
    if (ord & MO_Release)
    {
        ThreadFenceRelease();
    }
    ptr_t result = ExchangeRelaxed(atom, x);
    if (ord & MO_Acquire)
    {
        ThreadFenceAcquire();
    }
    return result;
}

inline ptr_t FetchAdd(aPtr& atom, isize x, MemOrder ord)
{
    if (ord & MO_Release)
    {
        ThreadFenceRelease();
    }
    ptr_t result = FetchAddRelaxed(atom, x);
    if (ord & MO_Acquire)
    {
        ThreadFenceAcquire();
    }
    return result;
}
