#pragma once

#include "common/macro.h"
#include "common/int_types.h"
#include <string.h>

template<typename T, u32 t_capacity>
struct RingBuf
{
    T m_data[t_capacity];
    u32 m_tail;
    u32 m_count;

    static constexpr u32 t_mask = t_capacity - 1u;

    static inline u32 Mask(u32 i) { return i & t_mask; }
    static inline u32 Next(u32 i) { return Mask(i + 1u); }
    static inline u32 Prev(u32 i) { return Mask(i - 1u); }

    inline u32 capacity() const { return t_mask; }
    inline usize capacityBytes() const { return (usize)t_mask * sizeof(T); }
    inline u32 size() const { return m_count; }
    inline usize sizeBytes() const { return (usize)size() * sizeof(T); }
    inline bool full() const { return size() == capacity(); }
    inline bool empty() const { return size() == 0u; }

    inline T& operator[](u32 i) { return m_data[Mask(i)]; }
    inline const T& operator[](u32 i) const { return m_data[Mask(i)]; }

    inline void Reset() { memset(this, 0, sizeof(*this)); }
    inline void Clear()
    {
        m_tail = 0;
        m_count = 0;
    }
    inline void Push(T value)
    {
        DebugAssert(!full());
        m_count++;
        m_data[Mask(m_tail++)] = value;
    }
    inline void Overwrite(T value)
    {
        u32 c = m_count + 1u;
        m_count = c > t_mask ? t_mask : c;
        m_data[Mask(m_tail++)] = value;
    }
    inline T Pop()
    {
        DebugAssert(!empty());
        u32 b = --m_count;
        return m_data[Mask(m_tail - b)];
    }
    inline u32 Begin() const { return Mask(m_tail - m_count); }
    inline u32 End() const { return Mask(m_tail); }
    inline u32 RBegin() const { return Prev(End()); }
    inline u32 REnd() const { return Next(Begin()); }
};
