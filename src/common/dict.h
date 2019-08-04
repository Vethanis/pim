#include "common/array.h"
#include "common/find.h"

template<typename T>
struct Dict
{
    Array<u32> keys;
    Array<T> values;

    inline void init(AllocatorType type) { keys.init(type); values.init(type); }
    inline void clear() { keys.clear(); values.clear(); }
    inline void reset() { keys.reset(); values.reset(); }
    inline i32 size() const { return keys.size(); }
    inline bool empty() const { return keys.empty(); }
    inline T* get(u32 key)
    {
        i32 i = RFind(keys.begin(), keys.size(), key);
        if(i == -1) { return 0; }
        return values.begin() + i;
    }
    inline void set(u32 key, const T& value)
    {
        if(T* ptr = get(key)) { *ptr = value; return; }
        keys.grow() = key; values.grow() = value;
    }
    inline void remove(u32 key)
    {
        i32 i = RFind(keys.begin(), keys.size(), key);
        if(i != -1) { keys.remove(i); values.remove(i); }
    }
    inline T& operator[](u32 key)
    {
        i32 i = Find(keys.begin(), keys.size(), key);
        if(i != -1) { return values[i]; }
        keys.grow() = key; values.grow(); return values.back();
    }

    struct iterator
    {
        u32* keys;
        T* values;
        const i32 len;
        i32 idx;

        inline operator++() { if(idx < len) { ++idx; } }
        inline bool operator!=(const iterator& other) const { return idx != other.idx; }
        u32& key() { return keys[idx]; }
        T& value() { return values[idx]; }
    };
    struct const_iterator
    {
        const u32* keys;
        const T* values;
        const i32 len;
        i32 idx;

        inline operator++() { if(idx < len) { ++idx; } }
        inline bool operator!=(const const_iterator& other) const { return idx != other.idx; }
        const u32& key() const { return keys[idx]; }
        const T& value() const { return values[idx]; }
    };

    inline iterator begin() { return { keys.begin(), values.begin(), size(), 0 }; }
    inline iterator end() { return { keys.begin(), values.begin(), size(), size() }; }
    inline const_iterator begin() const { return { keys.begin(), values.begin(), size(), 0 }; }
    inline const_iterator end() const { return { keys.begin(), values.begin(), size(), size() }; }
};
