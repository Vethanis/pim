#pragma once

#include "common/macro.h"
#include "common/int_types.h"

template<u32 t_capacity>
struct Ring
{
    u32 m_tail;
    u32 m_count;

    static constexpr u32 t_mask = t_capacity - 1u;

    static inline u32 Mask(u32 i) { return i & t_mask; }
    static inline u32 Next(u32 i) { return Mask(i + 1u); }
    static inline u32 Prev(u32 i) { return Mask(i - 1u); }
    static inline u32 RNext(u32 i) { return Mask(i - 1u); }
    static inline u32 RPrev(u32 i) { return Mask(i + 1u); }

    inline u32 capacity() const { return t_mask; }
    inline u32 size() const { return m_count; }
    inline bool full() const { return size() == capacity(); }
    inline bool empty() const { return size() == 0u; }

    inline void Clear()
    {
        m_tail = 0;
        m_count = 0;
    }
    inline u32 Push()
    {
        DebugAssert(!full());
        m_count++;
        return Mask(m_tail++);
    }
    inline u32 Overwrite()
    {
        u32 c = m_count + 1u;
        m_count = c > t_mask ? t_mask : c;
        return Mask(m_tail++);
    }
    inline u32 Pop()
    {
        DebugAssert(!empty());
        u32 b = --m_count;
        return Mask(m_tail - b);
    }
    inline u32 Begin() const { return Mask(m_tail - m_count); }
    inline u32 End() const { return Mask(m_tail); }
    inline u32 RBegin() const { return Mask(m_tail - 1u); }
    inline u32 REnd() const { return Mask(m_tail - m_count - 1u); }
};
