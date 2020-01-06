#include "os/atomics.h"

#include "common/macro.h"
#include <atomic>

// ----------------------------------------------------------------------------

SASSERT(MO_Relaxed == std::memory_order_relaxed);
SASSERT(MO_Consume == std::memory_order_consume);
SASSERT(MO_Acquire == std::memory_order_acquire);
SASSERT(MO_Release == std::memory_order_release);
SASSERT(MO_AcqRel == std::memory_order_acq_rel);
SASSERT(MO_SeqCst == std::memory_order_seq_cst);

static std::memory_order ToStd(MemOrder ord) noexcept
{
    return (std::memory_order)ord;
}

void AtomicSignalFence(MemOrder ord) noexcept
{
    std::atomic_signal_fence(ToStd(ord));
}

void AtomicThreadFence(MemOrder ord) noexcept
{
    std::atomic_thread_fence(ToStd(ord));
}

// ----------------------------------------------------------------------------

using StdAi32 = std::atomic_int32_t;

SASSERT(sizeof(Ai32) == sizeof(StdAi32));
SASSERT(alignof(Ai32) == alignof(StdAi32));

static const StdAi32& ToStd(const Ai32& self) noexcept
{
    return reinterpret_cast<const StdAi32&>(self);
}

static StdAi32& ToStd(Ai32& self) noexcept
{
    return reinterpret_cast<StdAi32&>(self);
}

static volatile StdAi32& ToStd(volatile Ai32& self) noexcept
{
    return reinterpret_cast<volatile StdAi32&>(self);
}

i32 Ai32::Load(MemOrder ord) const noexcept
{
    return ToStd(*this).load(ToStd(ord));
}

void Ai32::Store(i32 x, MemOrder ord) noexcept
{
    ToStd(*this).store(x, ToStd(ord));
}

bool Ai32::CmpExStrong(i32& cmp, i32 ex, MemOrder ord) noexcept
{
    return ToStd(*this).compare_exchange_strong(cmp, ex, ToStd(ord));
}

bool Ai32::CmpExWeak(i32& cmp, i32 ex, MemOrder ord) noexcept
{
    return ToStd(*this).compare_exchange_weak(cmp, ex, ToStd(ord));
}

bool Ai32::CmpExStrong(i32& cmp, i32 ex, MemOrder success, MemOrder failure) volatile noexcept
{
    return ToStd(*this).compare_exchange_strong(cmp, ex, ToStd(success), ToStd(failure));
}

bool Ai32::CmpExWeak(i32& cmp, i32 ex, MemOrder success, MemOrder failure) volatile noexcept
{
    return ToStd(*this).compare_exchange_weak(cmp, ex, ToStd(success), ToStd(failure));
}

i32 Ai32::Exchange(i32 ex, MemOrder ord) noexcept
{
    return ToStd(*this).exchange(ex, ToStd(ord));
}

i32 Ai32::CmpEx(i32& cmp, i32 ex, MemOrder ord) noexcept
{
    ToStd(*this).compare_exchange_strong(cmp, ex, ToStd(ord));
    return cmp;
}

i32 Ai32::Inc(MemOrder ord) noexcept
{
    return ToStd(*this)++;
}

i32 Ai32::Dec(MemOrder ord) noexcept
{
    return ToStd(*this)--;
}

i32 Ai32::Add(i32 x, MemOrder ord) noexcept
{
    return ToStd(*this).fetch_add(x, ToStd(ord));
}

i32 Ai32::Sub(i32 x, MemOrder ord) noexcept
{
    return ToStd(*this).fetch_sub(x, ToStd(ord));
}

i32 Ai32::Or(i32 x, MemOrder ord) noexcept
{
    return ToStd(*this).fetch_or(x, ToStd(ord));
}

i32 Ai32::And(i32 x, MemOrder ord) noexcept
{
    return ToStd(*this).fetch_and(x, ToStd(ord));
}

i32 Ai32::Xor(i32 x, MemOrder ord) noexcept
{
    return ToStd(*this).fetch_xor(x, ToStd(ord));
}

// ----------------------------------------------------------------------------

using StdAu32 = std::atomic_uint32_t;

SASSERT(sizeof(Au32) == sizeof(StdAu32));
SASSERT(alignof(Au32) == alignof(StdAu32));

static const StdAu32& ToStd(const Au32& self) noexcept
{
    return reinterpret_cast<const StdAu32&>(self);
}

static StdAu32& ToStd(Au32& self) noexcept
{
    return reinterpret_cast<StdAu32&>(self);
}

static volatile StdAu32& ToStd(volatile Au32& self) noexcept
{
    return reinterpret_cast<volatile StdAu32&>(self);
}

u32 Au32::Load(MemOrder ord) const noexcept
{
    return ToStd(*this).load(ToStd(ord));
}

void Au32::Store(u32 x, MemOrder ord) noexcept
{
    ToStd(*this).store(x, ToStd(ord));
}

bool Au32::CmpExStrong(u32& cmp, u32 ex, MemOrder ord) noexcept
{
    return ToStd(*this).compare_exchange_strong(cmp, ex, ToStd(ord));
}

bool Au32::CmpExWeak(u32& cmp, u32 ex, MemOrder ord) noexcept
{
    return ToStd(*this).compare_exchange_weak(cmp, ex, ToStd(ord));
}

bool Au32::CmpExStrong(u32& cmp, u32 ex, MemOrder success, MemOrder failure) volatile noexcept
{
    return ToStd(*this).compare_exchange_strong(cmp, ex, ToStd(success), ToStd(failure));
}

bool Au32::CmpExWeak(u32& cmp, u32 ex, MemOrder success, MemOrder failure) volatile noexcept
{
    return ToStd(*this).compare_exchange_weak(cmp, ex, ToStd(success), ToStd(failure));
}

u32 Au32::Exchange(u32 ex, MemOrder ord) noexcept
{
    return ToStd(*this).exchange(ex, ToStd(ord));
}

u32 Au32::CmpEx(u32& cmp, u32 ex, MemOrder ord) noexcept
{
    ToStd(*this).compare_exchange_strong(cmp, ex, ToStd(ord));
    return cmp;
}

u32 Au32::Inc(MemOrder ord) noexcept
{
    return ToStd(*this)++;
}

u32 Au32::Dec(MemOrder ord) noexcept
{
    return ToStd(*this)--;
}

u32 Au32::Add(u32 x, MemOrder ord) noexcept
{
    return ToStd(*this).fetch_add(x, ToStd(ord));
}

u32 Au32::Sub(u32 x, MemOrder ord) noexcept
{
    return ToStd(*this).fetch_sub(x, ToStd(ord));
}

u32 Au32::Or(u32 x, MemOrder ord) noexcept
{
    return ToStd(*this).fetch_or(x, ToStd(ord));
}

u32 Au32::And(u32 x, MemOrder ord) noexcept
{
    return ToStd(*this).fetch_and(x, ToStd(ord));
}

u32 Au32::Xor(u32 x, MemOrder ord) noexcept
{
    return ToStd(*this).fetch_xor(x, ToStd(ord));
}

// ----------------------------------------------------------------------------

using StdAi64 = std::atomic_int64_t;

SASSERT(sizeof(Ai64) == sizeof(StdAi64));
SASSERT(alignof(Ai64) == alignof(StdAi64));

static const StdAi64& ToStd(const Ai64& self) noexcept
{
    return reinterpret_cast<const StdAi64&>(self);
}

static StdAi64& ToStd(Ai64& self) noexcept
{
    return reinterpret_cast<StdAi64&>(self);
}

static volatile StdAi64& ToStd(volatile Ai64& self) noexcept
{
    return reinterpret_cast<volatile StdAi64&>(self);
}

i64 Ai64::Load(MemOrder ord) const noexcept
{
    return ToStd(*this).load(ToStd(ord));
}

void Ai64::Store(i64 x, MemOrder ord) noexcept
{
    ToStd(*this).store(x, ToStd(ord));
}

bool Ai64::CmpExStrong(i64& cmp, i64 ex, MemOrder ord) noexcept
{
    return ToStd(*this).compare_exchange_strong(cmp, ex, ToStd(ord));
}

bool Ai64::CmpExWeak(i64& cmp, i64 ex, MemOrder ord) noexcept
{
    return ToStd(*this).compare_exchange_weak(cmp, ex, ToStd(ord));
}

bool Ai64::CmpExStrong(i64& cmp, i64 ex, MemOrder success, MemOrder failure) volatile noexcept
{
    return ToStd(*this).compare_exchange_strong(cmp, ex, ToStd(success), ToStd(failure));
}

bool Ai64::CmpExWeak(i64& cmp, i64 ex, MemOrder success, MemOrder failure) volatile noexcept
{
    return ToStd(*this).compare_exchange_weak(cmp, ex, ToStd(success), ToStd(failure));
}

i64 Ai64::Exchange(i64 ex, MemOrder ord) noexcept
{
    return ToStd(*this).exchange(ex, ToStd(ord));
}

i64 Ai64::CmpEx(i64& cmp, i64 ex, MemOrder ord) noexcept
{
    ToStd(*this).compare_exchange_strong(cmp, ex, ToStd(ord));
    return cmp;
}

i64 Ai64::Inc(MemOrder ord) noexcept
{
    return ToStd(*this)++;
}

i64 Ai64::Dec(MemOrder ord) noexcept
{
    return ToStd(*this)--;
}

i64 Ai64::Add(i64 x, MemOrder ord) noexcept
{
    return ToStd(*this).fetch_add(x, ToStd(ord));
}

i64 Ai64::Sub(i64 x, MemOrder ord) noexcept
{
    return ToStd(*this).fetch_sub(x, ToStd(ord));
}

i64 Ai64::Or(i64 x, MemOrder ord) noexcept
{
    return ToStd(*this).fetch_or(x, ToStd(ord));
}

i64 Ai64::And(i64 x, MemOrder ord) noexcept
{
    return ToStd(*this).fetch_and(x, ToStd(ord));
}

i64 Ai64::Xor(i64 x, MemOrder ord) noexcept
{
    return ToStd(*this).fetch_xor(x, ToStd(ord));
}

// ----------------------------------------------------------------------------

using StdAu64 = std::atomic_uint64_t;

SASSERT(sizeof(Au64) == sizeof(StdAu64));
SASSERT(alignof(Au64) == alignof(StdAu64));

static const StdAu64& ToStd(const Au64& self) noexcept
{
    return reinterpret_cast<const StdAu64&>(self);
}

static StdAu64& ToStd(Au64& self) noexcept
{
    return reinterpret_cast<StdAu64&>(self);
}

static volatile StdAu64& ToStd(volatile Au64& self) noexcept
{
    return reinterpret_cast<volatile StdAu64&>(self);
}

u64 Au64::Load(MemOrder ord) const noexcept
{
    return ToStd(*this).load(ToStd(ord));
}

void Au64::Store(u64 x, MemOrder ord) noexcept
{
    ToStd(*this).store(x, ToStd(ord));
}

bool Au64::CmpExStrong(u64& cmp, u64 ex, MemOrder ord) noexcept
{
    return ToStd(*this).compare_exchange_strong(cmp, ex, ToStd(ord));
}

bool Au64::CmpExWeak(u64& cmp, u64 ex, MemOrder ord) noexcept
{
    return ToStd(*this).compare_exchange_weak(cmp, ex, ToStd(ord));
}

bool Au64::CmpExStrong(u64& cmp, u64 ex, MemOrder success, MemOrder failure) volatile noexcept
{
    return ToStd(*this).compare_exchange_strong(cmp, ex, ToStd(success), ToStd(failure));
}

bool Au64::CmpExWeak(u64& cmp, u64 ex, MemOrder success, MemOrder failure) volatile noexcept
{
    return ToStd(*this).compare_exchange_weak(cmp, ex, ToStd(success), ToStd(failure));
}

u64 Au64::Exchange(u64 ex, MemOrder ord) noexcept
{
    return ToStd(*this).exchange(ex, ToStd(ord));
}

u64 Au64::CmpEx(u64& cmp, u64 ex, MemOrder ord) noexcept
{
    ToStd(*this).compare_exchange_strong(cmp, ex, ToStd(ord));
    return cmp;
}

u64 Au64::Inc(MemOrder ord) noexcept
{
    return ToStd(*this)++;
}

u64 Au64::Dec(MemOrder ord) noexcept
{
    return ToStd(*this)--;
}

u64 Au64::Add(u64 x, MemOrder ord) noexcept
{
    return ToStd(*this).fetch_add(x, ToStd(ord));
}

u64 Au64::Sub(u64 x, MemOrder ord) noexcept
{
    return ToStd(*this).fetch_sub(x, ToStd(ord));
}

u64 Au64::Or(u64 x, MemOrder ord) noexcept
{
    return ToStd(*this).fetch_or(x, ToStd(ord));
}

u64 Au64::And(u64 x, MemOrder ord) noexcept
{
    return ToStd(*this).fetch_and(x, ToStd(ord));
}

u64 Au64::Xor(u64 x, MemOrder ord) noexcept
{
    return ToStd(*this).fetch_xor(x, ToStd(ord));
}

// ----------------------------------------------------------------------------

using StdAPtr = std::atomic_intptr_t;

SASSERT(sizeof(APtr) == sizeof(StdAPtr));
SASSERT(alignof(APtr) == alignof(StdAPtr));

static const StdAPtr& ToStd(const APtr& self) noexcept
{
    return reinterpret_cast<const StdAPtr&>(self);
}

static StdAPtr& ToStd(APtr& self) noexcept
{
    return reinterpret_cast<StdAPtr&>(self);
}

static volatile StdAPtr& ToStd(volatile APtr& self) noexcept
{
    return reinterpret_cast<volatile StdAPtr&>(self);
}

intptr_t APtr::Load(MemOrder ord) const noexcept
{
    return ToStd(*this).load(ToStd(ord));
}

void APtr::Store(intptr_t x, MemOrder ord) noexcept
{
    ToStd(*this).store(x, ToStd(ord));
}

bool APtr::CmpExStrong(intptr_t& cmp, intptr_t ex, MemOrder ord) noexcept
{
    return ToStd(*this).compare_exchange_strong(cmp, ex, ToStd(ord));
}

bool APtr::CmpExWeak(intptr_t& cmp, intptr_t ex, MemOrder ord) noexcept
{
    return ToStd(*this).compare_exchange_weak(cmp, ex, ToStd(ord));
}

bool APtr::CmpExStrong(intptr_t& cmp, intptr_t ex, MemOrder success, MemOrder failure) volatile noexcept
{
    return ToStd(*this).compare_exchange_strong(cmp, ex, ToStd(success), ToStd(failure));
}

bool APtr::CmpExWeak(intptr_t& cmp, intptr_t ex, MemOrder success, MemOrder failure) volatile noexcept
{
    return ToStd(*this).compare_exchange_weak(cmp, ex, ToStd(success), ToStd(failure));
}

intptr_t APtr::Exchange(intptr_t ex, MemOrder ord) noexcept
{
    return ToStd(*this).exchange(ex, ToStd(ord));
}

intptr_t APtr::CmpEx(intptr_t& cmp, intptr_t ex, MemOrder ord) noexcept
{
    ToStd(*this).compare_exchange_strong(cmp, ex, ToStd(ord));
    return cmp;
}

intptr_t APtr::Add(intptr_t x, MemOrder ord) noexcept
{
    return ToStd(*this).fetch_add(x, ToStd(ord));
}

intptr_t APtr::Sub(intptr_t x, MemOrder ord) noexcept
{
    return ToStd(*this).fetch_sub(x, ToStd(ord));
}
