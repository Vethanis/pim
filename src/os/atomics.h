#pragma once

#include "common/int_types.h"

enum MemOrder : i32
{
    MO_Relaxed, // no memory fencing, best for Inc(refCount)
    MO_Consume, // 'volatile'
    MO_Acquire, // Read -> Acquire
    MO_Release, // Store -> Release
    MO_AcqRel,  // RWR -> Acquire and Release
    MO_SeqCst,  // Sequentially consistent
};

void AtomicSignalFence(MemOrder ord) noexcept;
void AtomicThreadFence(MemOrder ord) noexcept;

struct Ai32
{
    i32 m_value;

    i32 Load(MemOrder ord = MO_SeqCst) const noexcept;
    void Store(i32 x, MemOrder ord = MO_SeqCst) noexcept;

    bool CmpExStrong(i32& cmp, i32 ex, MemOrder ord = MO_SeqCst) noexcept;
    bool CmpExWeak(i32& cmp, i32 ex, MemOrder ord = MO_SeqCst) noexcept;

    bool CmpExStrong(i32& cmp, i32 ex, MemOrder success, MemOrder failure) volatile noexcept;
    bool CmpExWeak(i32& cmp, i32 ex, MemOrder success, MemOrder failure) volatile noexcept;

    // returns previous value
    i32 Exchange(i32 ex, MemOrder ord = MO_SeqCst) noexcept;
    i32 CmpEx(i32& cmp, i32 ex, MemOrder ord = MO_SeqCst) noexcept;
    i32 Inc(MemOrder ord = MO_SeqCst) noexcept;
    i32 Dec(MemOrder ord = MO_SeqCst) noexcept;
    i32 Add(i32 x, MemOrder ord = MO_SeqCst) noexcept;
    i32 Sub(i32 x, MemOrder ord = MO_SeqCst) noexcept;
    i32 Or(i32 x, MemOrder ord = MO_SeqCst) noexcept;
    i32 And(i32 x, MemOrder ord = MO_SeqCst) noexcept;
    i32 Xor(i32 x, MemOrder ord = MO_SeqCst) noexcept;

    inline Ai32& operator=(const Ai32& rhs) noexcept
    {
        Store(rhs.Load());
        return *this;
    }

    inline Ai32& operator=(i32 x) noexcept
    {
        Store(x);
        return *this;
    }
};

struct Au32
{
    u32 m_value;

    u32 Load(MemOrder ord = MO_SeqCst) const noexcept;
    void Store(u32 x, MemOrder ord = MO_SeqCst) noexcept;

    bool CmpExStrong(u32& cmp, u32 ex, MemOrder ord = MO_SeqCst) noexcept;
    bool CmpExWeak(u32& cmp, u32 ex, MemOrder ord = MO_SeqCst) noexcept;

    bool CmpExStrong(u32& cmp, u32 ex, MemOrder success, MemOrder failure) volatile noexcept;
    bool CmpExWeak(u32& cmp, u32 ex, MemOrder success, MemOrder failure) volatile noexcept;

    // returns previous value
    u32 Exchange(u32 ex, MemOrder ord = MO_SeqCst) noexcept;
    u32 CmpEx(u32& cmp, u32 ex, MemOrder ord = MO_SeqCst) noexcept;
    u32 Inc(MemOrder ord = MO_SeqCst) noexcept;
    u32 Dec(MemOrder ord = MO_SeqCst) noexcept;
    u32 Add(u32 x, MemOrder ord = MO_SeqCst) noexcept;
    u32 Sub(u32 x, MemOrder ord = MO_SeqCst) noexcept;
    u32 Or(u32 x, MemOrder ord = MO_SeqCst) noexcept;
    u32 And(u32 x, MemOrder ord = MO_SeqCst) noexcept;
    u32 Xor(u32 x, MemOrder ord = MO_SeqCst) noexcept;

    inline Au32& operator=(const Au32& rhs) noexcept
    {
        Store(rhs.Load());
        return *this;
    }

    inline Au32& operator=(u32 x) noexcept
    {
        Store(x);
        return *this;
    }
};

struct Au64
{
    u64 m_value;

    u64 Load(MemOrder ord = MO_SeqCst) const noexcept;
    void Store(u64 x, MemOrder ord = MO_SeqCst) noexcept;

    bool CmpExStrong(u64& cmp, u64 ex, MemOrder ord = MO_SeqCst) noexcept;
    bool CmpExWeak(u64& cmp, u64 ex, MemOrder ord = MO_SeqCst) noexcept;

    bool CmpExStrong(u64& cmp, u64 ex, MemOrder success, MemOrder failure) volatile noexcept;
    bool CmpExWeak(u64& cmp, u64 ex, MemOrder success, MemOrder failure) volatile noexcept;

    // returns previous value
    u64 Exchange(u64 ex, MemOrder ord = MO_SeqCst) noexcept;
    u64 CmpEx(u64& cmp, u64 ex, MemOrder ord = MO_SeqCst) noexcept;
    u64 Inc(MemOrder ord = MO_SeqCst) noexcept;
    u64 Dec(MemOrder ord = MO_SeqCst) noexcept;
    u64 Add(u64 x, MemOrder ord = MO_SeqCst) noexcept;
    u64 Sub(u64 x, MemOrder ord = MO_SeqCst) noexcept;
    u64 Or(u64 x, MemOrder ord = MO_SeqCst) noexcept;
    u64 And(u64 x, MemOrder ord = MO_SeqCst) noexcept;
    u64 Xor(u64 x, MemOrder ord = MO_SeqCst) noexcept;

    inline Au64& operator=(const Au64& rhs) noexcept
    {
        Store(rhs.Load());
        return *this;
    }

    inline Au64& operator=(u64 x) noexcept
    {
        Store(x);
        return *this;
    }
};

struct Ai64
{
    i64 m_value;

    i64 Load(MemOrder ord = MO_SeqCst) const noexcept;
    void Store(i64 x, MemOrder ord = MO_SeqCst) noexcept;

    bool CmpExStrong(i64& cmp, i64 ex, MemOrder ord = MO_SeqCst) noexcept;
    bool CmpExWeak(i64& cmp, i64 ex, MemOrder ord = MO_SeqCst) noexcept;

    bool CmpExStrong(i64& cmp, i64 ex, MemOrder success, MemOrder failure) volatile noexcept;
    bool CmpExWeak(i64& cmp, i64 ex, MemOrder success, MemOrder failure) volatile noexcept;

    // returns previous value
    i64 Exchange(i64 ex, MemOrder ord = MO_SeqCst) noexcept;
    i64 CmpEx(i64& cmp, i64 ex, MemOrder ord = MO_SeqCst) noexcept;
    i64 Inc(MemOrder ord = MO_SeqCst) noexcept;
    i64 Dec(MemOrder ord = MO_SeqCst) noexcept;
    i64 Add(i64 x, MemOrder ord = MO_SeqCst) noexcept;
    i64 Sub(i64 x, MemOrder ord = MO_SeqCst) noexcept;
    i64 Or(i64 x, MemOrder ord = MO_SeqCst) noexcept;
    i64 And(i64 x, MemOrder ord = MO_SeqCst) noexcept;
    i64 Xor(i64 x, MemOrder ord = MO_SeqCst) noexcept;

    inline Ai64& operator=(const Ai64& rhs) noexcept
    {
        Store(rhs.Load());
        return *this;
    }

    inline Ai64& operator=(i64 x) noexcept
    {
        Store(x);
        return *this;
    }
};

struct APtr
{
    intptr_t m_value;

    intptr_t Load(MemOrder ord = MO_SeqCst) const noexcept;
    void Store(intptr_t x, MemOrder ord = MO_SeqCst) noexcept;

    bool CmpExStrong(intptr_t& cmp, intptr_t ex, MemOrder ord = MO_SeqCst) noexcept;
    bool CmpExWeak(intptr_t& cmp, intptr_t ex, MemOrder ord = MO_SeqCst) noexcept;

    bool CmpExStrong(intptr_t& cmp, intptr_t ex, MemOrder success, MemOrder failure) volatile noexcept;
    bool CmpExWeak(intptr_t& cmp, intptr_t ex, MemOrder success, MemOrder failure) volatile noexcept;

    // returns previous value
    intptr_t Exchange(intptr_t ex, MemOrder ord = MO_SeqCst) noexcept;
    intptr_t CmpEx(intptr_t& cmp, intptr_t ex, MemOrder ord = MO_SeqCst) noexcept;
    intptr_t Add(intptr_t x, MemOrder ord = MO_SeqCst) noexcept;
    intptr_t Sub(intptr_t x, MemOrder ord = MO_SeqCst) noexcept;

    inline APtr& operator=(const APtr& rhs) noexcept
    {
        Store(rhs.Load());
        return *this;
    }

    inline APtr& operator=(intptr_t x) noexcept
    {
        Store(x);
        return *this;
    }
};

template<typename T>
struct ATPtr
{
    APtr m_ptr;

    static T* ToPtr(intptr_t addr) noexcept { return reinterpret_cast<T*>(addr); }
    static intptr_t ToAddr(T* ptr) noexcept { return reinterpret_cast<intptr_t>(ptr); }
    static intptr_t& ToAddrRef(T*& rPtr) noexcept { return reinterpret_cast<intptr_t&>(rPtr); }

    T* Load(MemOrder ord = MO_SeqCst) const noexcept
    {
        return ToPtr(m_ptr.Load(ord));
    }
    void Store(T* x, MemOrder ord = MO_SeqCst) noexcept
    {
        m_ptr.Store(ToAddr(x), ord);
    }

    bool CmpExStrong(T*& cmp, T* ex, MemOrder ord = MO_SeqCst) noexcept
    {
        return m_ptr.CmpExStrong(ToAddrRef(cmp), ToAddr(ex), ord);
    }
    bool CmpExWeak(T*& cmp, T* ex, MemOrder ord = MO_SeqCst) noexcept
    {
        return m_ptr.CmpExWeak(ToAddrRef(cmp), ToAddr(ex), ord);
    }

    bool CmpExStrong(T*& cmp, T* ex, MemOrder success, MemOrder failure) volatile noexcept
    {
        return m_ptr.CmpExStrong(ToAddrRef(cmp), ToAddr(ex), success, failure);
    }
    bool CmpExWeak(T*& cmp, T* ex, MemOrder success, MemOrder failure) volatile noexcept
    {
        return m_ptr.CmpExWeak(ToAddrRef(cmp), ToAddr(ex), success, failure);
    }

    // returns previous value
    T* Exchange(T* ex, MemOrder ord = MO_SeqCst) noexcept
    {
        return ToPtr(m_ptr.Exchange(ToAddr(ex), ord));
    }
    T* CmpEx(T*& cmp, T* ex, MemOrder ord = MO_SeqCst) noexcept
    {
        return ToPtr(m_ptr.CmpEx(ToAddrRef(cmp), ToAddr(ex), ord));
    }
    T* Inc(MemOrder ord = MO_SeqCst) noexcept
    {
        return ToPtr(m_ptr.Add(sizeof(T), ord));
    }
    T* Dec(MemOrder ord = MO_SeqCst) noexcept
    {
        return ToPtr(m_ptr.Sub(sizeof(T), ord));
    }
    T* Add(isize x, MemOrder ord = MO_SeqCst) noexcept
    {
        return ToPtr(m_ptr.Add(x * sizeof(T), ord));
    }
    T* Sub(isize x, MemOrder ord = MO_SeqCst) noexcept
    {
        return ToPtr(m_ptr.Sub(x * sizeof(T), ord));
    }

    inline ATPtr& operator=(const ATPtr& rhs) noexcept
    {
        Store(rhs.Load());
        return *this;
    }

    inline ATPtr& operator=(T* x) noexcept
    {
        Store(x);
        return *this;
    }
};
