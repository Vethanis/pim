#include "containers/array.h"

template<typename K, typename V>
struct Dict
{
    Array<K> m_keys;
    Array<V> m_values;

    inline i32 size() const { return m_keys.size(); }
    inline i32 capacity() const { return m_keys.capacity(); }
    inline bool empty() const { return m_keys.empty(); }
    inline bool full() const { return m_keys.full(); }

    inline void init(AllocatorType type)
    {
        m_keys.init(type);
        m_values.init(type);
    }
    inline void reset()
    {
        m_keys.reset();
        m_values.reset();
    }
    inline void clear()
    {
        m_keys.clear();
        m_values.clear();
    }
    inline i32 find(K key) const
    {
        const K* pKeys = m_keys.begin();
        const i32 len = m_keys.size();
        for (i32 i = 0; i < len; ++i)
        {
            if (pKeys[i] == key)
            {
                return i;
            }
        }
        return -1;
    }
    inline bool contains(K key) const { return find(key) != -1; }
    inline const V* get(K key) const
    {
        const i32 i = find(key);
        return (i == -1) ? 0 : m_values.begin() + i;
    }
    inline V* get(K key)
    {
        const i32 i = find(key);
        return (i == -1) ? 0 : m_values.begin() + i;
    }
    inline void remove(K key)
    {
        const i32 i = find(key);
        if (i != -1)
        {
            m_keys.remove(i);
            m_values.remove(i);
        }
    }
    inline V& operator[](K key)
    {
        const i32 i = find(key);
        if (i != -1)
        {
            return m_values[i];
        }
        else
        {
            m_keys.grow() = key;
            return m_values.grow();
        }
    }
};
