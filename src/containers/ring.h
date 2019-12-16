#pragma once

#include "common/macro.h"
#include "common/int_types.h"

template<u32 t_capacity>
struct Ring
{
    u32 m_tail;
    u32 m_count;

    static constexpr u32 t_mask = t_capacity - 1u;

    SASSERT((t_capacity & (t_capacity - 1u)) == 0);

    static inline u32 Mask(u32 i) { return i & t_mask; }

    inline u32 Capacity() const { return t_capacity; }
    inline u32 Size() const { return m_count; }
    inline bool IsFull() const { return m_count >= t_capacity; }
    inline bool IsEmpty() const { return !m_count; }

    inline void Clear()
    {
        m_tail = 0u;
        m_count = 0u;
    }
    inline u32 Push()
    {
        ASSERT(!IsFull());
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
        ASSERT(!IsEmpty());
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
        inline bool operator!=(iterator o) const
        {
            return m_left != 0;
        }
        inline u32 operator *() const
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
            return *this;
        }
        inline bool operator!=(riterator o) const
        {
            return m_left != 0;
        }
        inline u32 operator *() const
        {
            return Mask(m_i);
        }
    };

    inline iterator begin() const { return iterator(m_count, m_tail); }
    inline iterator end() const { return { 0, 0 }; }
    inline riterator rbegin() const { return riterator(m_count, m_tail); }
    inline riterator rend() const { return { 0, 0 }; }
};

template<typename T, u32 t_capacity>
struct RingBuffer
{
    Ring<t_capacity> m_ring;
    T m_data[t_capacity];

    inline u32 Capacity() const { return m_ring.Capacity(); }
    inline u32 Size() const { return m_ring.Size(); }
    inline bool IsFull() const { return m_ring.IsFull(); }
    inline bool IsEmpty() const { return m_ring.IsEmpty(); }

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

    // ------------------------------------------------------------------------

    struct iterator
    {
        decltype(m_ring.begin()) it;
        RingBuffer<T, t_capacity>& buffer;

        inline iterator& operator++()
        {
            ++it;
            return *this;
        }
        inline bool operator!=(iterator o) const
        {
            return it != o.it;
        }
        inline T& operator *()
        {
            return buffer.m_data[*it];
        }
    };
    inline iterator begin()
    {
        return { m_ring.begin(), *this };
    }
    inline iterator end()
    {
        return { m_ring.end(), *this };
    }

    // ------------------------------------------------------------------------

    struct citerator
    {
        decltype(m_ring.begin()) it;
        const RingBuffer<T, t_capacity>& buffer;

        inline citerator& operator++()
        {
            ++it;
            return *this;
        }
        inline bool operator!=(citerator o) const
        {
            return it != o.it;
        }
        inline const T& operator *() const
        {
            return buffer.m_data[*it];
        }
    };
    inline citerator begin() const
    {
        return { m_ring.begin(), *this };
    }
    inline citerator end() const
    {
        return { m_ring.end(), *this };
    }

    // ------------------------------------------------------------------------

    struct riterator
    {
        decltype(m_ring.rbegin()) it;
        RingBuffer<T, t_capacity>& buffer;

        inline riterator& operator++()
        {
            ++it;
            return *this;
        }
        inline bool operator!=(riterator o) const
        {
            return it != o.it;
        }
        inline T& operator *()
        {
            return buffer.m_data[*it];
        }
    };
    inline riterator rbegin()
    {
        return { m_ring.rbegin(), *this };
    }
    inline riterator rend()
    {
        return { m_ring.rend(), *this };
    }

    // ------------------------------------------------------------------------

    struct rciterator
    {
        decltype(m_ring.rbegin()) it;
        const RingBuffer<T, t_capacity>& buffer;

        inline rciterator& operator++()
        {
            ++it;
            return *this;
        }
        inline bool operator!=(rciterator o) const
        {
            return it != o.it;
        }
        inline const T& operator *() const
        {
            return buffer.m_data[*it];
        }
    };
    inline rciterator rbegin() const
    {
        return { m_ring.rbegin(), *this };
    }
    inline rciterator rend() const
    {
        return { m_ring.rend(), *this };
    }

    // ------------------------------------------------------------------------

};
