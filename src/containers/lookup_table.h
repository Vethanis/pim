#pragma once

#include "containers/array.h"
#include "containers/hash_dict.h"

// Acceleration structure for a struct of arrays.
template<typename T, const Comparator<T>& cmp>
struct LookupTable
{
    HashDict<T, i32, cmp> Lookup;
    Array<T> Table;

    inline void Init(AllocType allocator)
    {
        Lookup.Init(allocator);
        Table.Init(allocator);
    }

    inline void Reset()
    {
        Lookup.Reset();
        Table.Reset();
    }

    inline void Clear()
    {
        Lookup.Clear();
        Table.Clear();
    }

    inline void Trim()
    {
        Lookup.Trim();
        Table.Trim();
    }

    inline void Reserve(i32 capacity)
    {
        Lookup.Reserve(capacity);
        Table.Reserve(capacity);
    }

    // ------------------------------------------------------------------------

    inline i32 size() const { return Table.Size(); }
    inline T* begin() { return Table.begin(); }
    inline T* end() { return Table.end(); }
    inline const T* begin() const { return Table.begin(); }
    inline const T* end() const { return Table.end(); }

    // ------------------------------------------------------------------------

    inline const T& operator[](i32 i) const
    {
        return Table[i];
    }
    inline T& operator[](i32 i)
    {
        return Table[i];
    }

    // ------------------------------------------------------------------------

    inline AllocType GetAllocator() const { return Table.GetAllocType(); }

    inline bool Contains(T item) const
    {
        return Lookup.Contains(item);
    }

    inline i32 Find(T item) const
    {
        const i32* pIndex = Lookup.Get(item);
        return pIndex ? *pIndex : -1;
    }

    inline i32 Add(T item)
    {
        i32 i = -1;
        const i32 back = Table.Size();
        if (Lookup.Add(item, back))
        {
            Table.Grow() = item;
            i = back;
        }
        return i;
    }

    inline void RemoveAt(i32 i)
    {
        const i32 back = Table.Size() - 1;
        if (i != back)
        {
            Lookup.Set(Table[back], i);
        }
        Lookup.Remove(Table[i]);
        Table.Remove(i);
    }

    inline i32 Remove(T item)
    {
        const i32 i = Find(item);
        if (i != -1)
        {
            RemoveAt(i);
        }
        return i;
    }
};
