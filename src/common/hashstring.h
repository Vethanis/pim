#pragma once

#include "common/macro.h"
#include "common/int_types.h"
#include "common/hash.h"
#include "common/stringutil.h"
#include "containers/slice.h"

#define HashNSBits  (5)
#define HashNSCount (1 << HashNSBits)
#define HashNSMask  (HashNSCount - 1)
#define DeclareNS(T) static const u8 NS_##T = (HashString::NSIdx++ & HashNSMask)

using HashNamespace = u8;
using HashStringKey = u32;

static constexpr HashStringKey StrHash(HashNamespace ns, cstrc src)
{
    if (!src) { return 0; }

    HashStringKey hash = Fnv32_Bias;
    for (usize i = 0; src[i]; ++i)
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

    static Slice<char> Lookup(HashStringKey key);
    static HashStringKey Insert(HashNamespace ns, cstrc value);
    static void Reset();

    inline HashString() : m_key(0) {}
    inline bool IsNull() const { return !(m_key >> HashNSBits); }
    inline HashString(HashNamespace ns, cstrc src)
    {
        m_key = Insert(ns, src);
    }
    inline Slice<char> Get() const
    {
        return Lookup(m_key);
    }

    static u8 NSIdx;
};

inline bool operator==(HashString a, HashString b)
{
    return a.m_key == b.m_key;
}
inline bool operator!=(HashString a, HashString b)
{
    return a.m_key != b.m_key;
}
