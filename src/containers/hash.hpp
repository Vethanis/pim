#pragma once

#include "containers/hash_util.h"

template<typename T>
struct Hashable
{
    static u32 HashOf(T const& item)
    {
        return hashutil_hash(&item, sizeof(T));
    }
};

template<>
struct Hashable<char>
{
    static u32 HashOf(char const& item)
    {
        return hashutil_create_hash(Fnv32Char(item, Fnv32Bias));
    }
};
template<>
struct Hashable<u8>
{
    static u32 HashOf(u8 const& item)
    {
        return hashutil_create_hash(Fnv32Byte(item, Fnv32Bias));
    }
};
template<>
struct Hashable<i8>
{
    static u32 HashOf(i8 const& item) { return Hashable<u8>::HashOf((u8)item); }
};

template<>
struct Hashable<u16>
{
    static u32 HashOf(u16 const& item)
    {
        return hashutil_create_hash(Fnv32Word(item, Fnv32Bias));
    }
};
template<>
struct Hashable<i16>
{
    static u32 HashOf(i16 const& item) { return Hashable<u16>::HashOf((u16)item); }
};

template<>
struct Hashable<u32>
{
    static u32 HashOf(u32 const& item)
    {
        return hashutil_create_hash(Fnv32Dword(item, Fnv32Bias));
    }
};
template<>
struct Hashable<i32>
{
    static u32 HashOf(i32 const& item) { return Hashable<u32>::HashOf((u32)item); }
};

template<>
struct Hashable<u64>
{
    static u32 HashOf(u64 const& item)
    {
        return hashutil_create_hash(Fnv32Qword(item, Fnv32Bias));
    }
};
template<>
struct Hashable<i64>
{
    static u32 HashOf(i64 const& item) { return Hashable<u64>::HashOf((u64)item); }
};

template<>
struct Hashable<char const*>
{
    static u32 HashOf(char const *const & item)
    {
        return hashutil_create_hash(HashStr(item));
    }
};
template<>
struct Hashable<char*>
{
    static u32 HashOf(char *const & item) { return Hashable<char const*>::HashOf(item); }
};
