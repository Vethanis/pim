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

    inline u32 capacity() const { return t_capacity; }
    inline u32 size() const { return m_count; }
    inline bool full() const { return m_count >= t_capacity; }
    inline bool empty() const { return !m_count; }

    inline void Clear()
    {
        m_tail = 0u;
        m_count = 0u;
    }
    inline u32 Push()
    {
        DebugAssert(!full());
        m_count++;
        return Mask(m_tail++);
    }
    inline u32 Overwrite()
    {
        m_count = Min(m_count + 1u, t_capacity);
        return Mask(m_tail++);
    }
    inline u32 Pop()
    {
        DebugAssert(!empty());
        return Mask(m_tail - (m_count--));
    }

    struct iterator
    {
        u32 m_left;
        u32 m_i;

        inline iterator(u32 count, u32 tail)
        {
            m_left = count;
            m_i = tail - count;
        }

        inline iterator& operator++()
        {
            --m_left;
            ++m_i;
            return *this;
        }
        inline bool operator!=(iterator o)
        {
            return m_left != 0;
        }
        inline operator u32()
        {
            return Mask(m_i);
        }
    };

    struct riterator
    {
        u32 m_left;
        u32 m_i;

        inline riterator(u32 count, u32 tail)
        {
            m_left = count;
            m_i = tail - 1;
        }

        inline riterator& operator++()
        {
            --m_left;
            --m_i;
        }
        inline bool operator!=(riterator o)
        {
            return m_left != 0;
        }
        inline operator u32()
        {
            return Mask(m_i);
        }
    };

    inline iterator begin() const { return iterator(m_count, m_tail); }
    inline iterator end() const { return {}; }
    inline riterator rbegin() const { return riterator(m_count, m_tail); }
    inline riterator rend() const { return {}; }
};

template<typename T, u32 t_capacity>
struct RingBuffer
{
    Ring<t_capacity> m_ring;
    T m_data[t_capacity];

    inline u32 capacity() const { return m_ring.capacity(); }
    inline u32 size() const { return m_ring.size(); }
    inline bool full() const { return m_ring.full(); }
    inline bool empty() const { return m_ring.empty(); }

    inline void Clear()
    {
        m_ring.Clear();
    }
    inline T& Push()
    {
        return m_data[m_ring.Push()];
    }
    inline T& Overwrite()
    {
        return m_data[m_ring.Overwrite()];
    }
    inline T& Pop()
    {
        return m_data[m_ring.Pop()];
    }
};
