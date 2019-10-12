#pragma once

#include "common/macro.h"
#include "common/int_types.h"
#include "common/hash.h"
#include "common/stringutil.h"

using HashNamespace = u8;
using HashStringKey = u32;

constexpr u32 HashNSBits = 5;
constexpr u32 HashNSCount = 1 << HashNSBits;
constexpr u32 HashNSMask = HashNSCount - 1;

inline constexpr u8 DeclareNS(cstrc name)
{
    return Fnv8String(name);
}

inline constexpr HashStringKey StrHash(HashNamespace ns, cstrc src)
{
    if (!src || !src[0]) { return 0; }

    HashStringKey hash = Fnv32_Bias;
    for (i32 i = 0; src[i]; ++i)
    {
        hash = Fnv32Byte(ToLower(src[i]), hash);
    }

    hash = hash ? hash : 1;
    hash = (hash << HashNSBits) | ns;

    return hash;
}

struct HashString
{
    HashStringKey m_key;

    inline HashString() : m_key(0) {}
    inline HashString(HashNamespace ns, HashStringKey key)
    {
        m_key = (key << HashNSBits) | ns;
    }
    inline HashString(HashNamespace ns, cstrc src)
    {
        m_key = StrHash(ns, src);
        IfDebug(Insert(ns, src);)
    }
    inline bool IsNull() const { return !(m_key >> HashNSBits); }

#if _DEBUG
    inline cstr Get() const
    {
        return Lookup(m_key);
    }
    static cstr Lookup(HashStringKey key);
    static void Insert(HashNamespace ns, cstrc value);
#endif // _DEBUG
};

inline bool operator==(HashString a, HashString b)
{
    return a.m_key == b.m_key;
}
inline bool operator!=(HashString a, HashString b)
{
    return a.m_key != b.m_key;
}
