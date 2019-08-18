#include "common/array.h"
#include "common/find.h"
#include "common/hash.h"

template<typename K, typename V>
struct Dict
{
    Array<u8> fuzz;
    Array<K> keys;
    Array<V> values;

    inline i32 size() const { return fuzz.size(); }
    inline i32 capacity() const { return fuzz.capacity(); }
    inline bool empty() const { return fuzz.empty(); }
    inline bool full() const { return fuzz.full(); }

    inline void init(AllocatorType type)
    {
        fuzz.init(type);
        keys.init(type);
        values.init(type);
    }
    inline void reset()
    {
        fuzz.reset();
        keys.reset();
        values.reset();
    }
    inline void clear()
    {
        fuzz.clear();
        keys.clear();
        values.clear();
    }
    inline i32 find(K key) const
    {
        const u8 keyFuzzed = Fnv8Bytes(&key, sizeof(K));
        const u8* pFuzz = fuzz.begin();
        const i32 len = fuzz.size();
        const K* pKeys = keys.begin();
        for (i32 i = 0; i < len; ++i)
        {
            if (pFuzz[i] == keyFuzzed)
            {
                if (pKeys[i] == key)
                {
                    return i;
                }
            }
        }
        return -1;
    }
    inline bool contains(K key) const { return find(key) != -1; }
    inline const V* get(K key) const
    {
        const i32 i = find(key);
        return (i == -1) ? 0 : values.begin() + i;
    }
    inline V* get(K key)
    {
        const i32 i = find(key);
        return (i == -1) ? 0 : values.begin() + i;
    }
    inline void remove(K key)
    {
        const i32 i = find(key);
        if (i != -1)
        {
            fuzz.remove(i);
            keys.remove(i);
            values.remove(i);
        }
    }
    inline V& operator[](K key)
    {
        const i32 i = find(key);
        if (i != -1)
        {
            return values[i];
        }
        else
        {
            fuzz.grow() = Fnv8Bytes(&key, sizeof(K));
            keys.grow() = key;
            return values.grow();
        }
    }
};
